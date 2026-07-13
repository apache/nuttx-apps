#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.

"""Export built binaries into a package repository layout.

The script copies built application or library artifacts into a
server-friendly repository directory, computes SHA-256 digests, and emits an
`index.json` compatible with the current `nxpkg` metadata parser.

The repository can then be served by FTP, HTTP, or any static file transport.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List

VALID_TYPES = {"elf", "shared-lib"}
PACKAGE_SPEC_HELP = (
    "package must look like " "<name>:<version>:<elf|shared-lib>:<source>"
)

# Fixed square icon size nxstore renders app-list icons at (see
# nxstore_load_icon() in system/nxstore/nxstore_main.c) - small enough to
# stay a quick download over the board's WiFi, large enough to read as
# actual artwork rather than a favicon.
ICON_SIZE = 48

# Matches lv_image_header_t's own bit layout (LVGL v9, see
# graphics/lvgl/lvgl/src/draw/lv_image_dsc.h): magic/cf/flags packed into
# the first 4 bytes, then w/h, then stride/reserved - all little-endian,
# 12 bytes total, immediately followed by raw pixel data.  This is
# deliberately the simplest format LVGL can render directly with no
# decoder, since the board has no PNG/JPEG decode capability wired up.
LV_IMAGE_HEADER_MAGIC = 0x19
LV_COLOR_FORMAT_RGB565 = 0x12


@dataclass
class PackageSpec:
    name: str
    version: str
    payload_type: str
    source: Path


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as infile:
        while True:
            chunk = infile.read(65536)
            if not chunk:
                break

            digest.update(chunk)

    return digest.hexdigest()


def parse_package_spec(value: str) -> PackageSpec:
    parts = value.split(":", 3)
    if len(parts) != 4:
        raise argparse.ArgumentTypeError(PACKAGE_SPEC_HELP)

    name, version, payload_type, source = parts
    if payload_type not in VALID_TYPES:
        raise argparse.ArgumentTypeError(
            f"unsupported package type '{payload_type}', expected one of "
            f"{', '.join(sorted(VALID_TYPES))}"
        )

    source_path = Path(source).expanduser().resolve()
    if not source_path.is_file():
        msg = f"source does not exist: {source_path}"
        raise argparse.ArgumentTypeError(msg)

    return PackageSpec(
        name=name,
        version=version,
        payload_type=payload_type,
        source=source_path,
    )


def parse_name_value(value: str) -> tuple:
    parts = value.split("=", 1)
    if len(parts) != 2 or not parts[0] or not parts[1]:
        raise argparse.ArgumentTypeError("value must look like <name>=<text>")

    return parts[0], parts[1]


def parse_icon_spec(value: str) -> tuple:
    name, source = parse_name_value(value)
    source_path = Path(source).expanduser().resolve()
    if not source_path.is_file():
        msg = f"icon source does not exist: {source_path}"
        raise argparse.ArgumentTypeError(msg)

    return name, source_path


def artifact_relpath(arch: str, chip: str, compat: str,
                     spec: PackageSpec) -> Path:  # fmt: skip
    filename = spec.source.name
    path = Path("artifacts") / arch / chip / compat
    path = path / spec.name / spec.version / filename
    return path


def icon_relpath(arch: str, chip: str, compat: str, name: str) -> Path:
    # Keep one repository object per package. nxstore keys its local cache
    # by package name and version, so a new catalog version fetches this
    # object again without requiring versioned repository duplication.
    return Path("icons") / arch / chip / compat / f"{name}.bin"


def encode_icon_rgb565(source: Path, size: int = ICON_SIZE) -> bytes:
    """Convert an arbitrary source image into nxstore's raw icon format:
    a 12-byte lv_image_header_t-compatible header (see the module
    docstring comment above) followed by size*size RGB565 pixel data.
    """

    from PIL import Image

    with Image.open(source) as img:
        img = img.convert("RGB").resize((size, size), Image.Resampling.LANCZOS)
        pixels = list(img.getdata())

    stride = size * 2
    header = struct.pack(
        "<BBHHHHH",
        LV_IMAGE_HEADER_MAGIC,
        LV_COLOR_FORMAT_RGB565,
        0,  # flags
        size,  # w
        size,  # h
        stride,
        0,  # reserved_2
    )

    body = bytearray(stride * size)
    offset = 0
    for r, g, b in pixels:
        rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        struct.pack_into("<H", body, offset, rgb565)
        offset += 2

    return header + bytes(body)


def package_identity(package: Dict[str, str]) -> tuple:
    return (
        package["name"],
        package["version"],
        package["arch"],
        package["compat"],
        package["type"],
    )


def load_existing_packages(repo_dir: Path) -> List[dict]:
    index_path = repo_dir / "index.json"

    if not index_path.exists():
        return []

    root = json.loads(index_path.read_text(encoding="utf-8"))
    if isinstance(root, list):
        packages = root
    else:
        packages = root.get("packages")

    if not isinstance(packages, list):
        raise ValueError(f"invalid repository index format in {index_path}")

    for item in packages:
        if not isinstance(item, dict):
            raise ValueError(f"invalid repository index entry in {index_path}")

    return packages


def emit_index(repo_dir: Path, packages: List[dict]) -> None:
    index_path = repo_dir / "index.json"
    payload = {"packages": packages}
    index_path.write_text(
        json.dumps(payload, indent=2, sort_keys=False) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "repo_dir",
        type=Path,
        help="Destination repository directory",
    )
    parser.add_argument(
        "--arch",
        required=True,
        help="Target architecture string, for example xtensa",
    )
    parser.add_argument(
        "--chip",
        required=True,
        help="Target chip/family string, for example esp32s3",
    )
    parser.add_argument(
        "--compat",
        required=True,
        help="Target board/runtime identity, for example esp32s3-xiao",
    )
    parser.add_argument(
        "--artifact-prefix",
        default="",
        help="Optional prefix prepended to artifact paths in index.json",
    )
    parser.add_argument(
        "--package",
        action="append",
        required=True,
        type=parse_package_spec,
        help=PACKAGE_SPEC_HELP,
    )
    parser.add_argument(
        "--package-description",
        action="append",
        default=[],
        type=parse_name_value,
        metavar="NAME=TEXT",
        help="Optional package description, keyed by package name",
    )
    parser.add_argument(
        "--package-category",
        action="append",
        default=[],
        type=parse_name_value,
        metavar="NAME=TEXT",
        help="Optional package category, keyed by package name",
    )
    parser.add_argument(
        "--package-icon",
        action="append",
        default=[],
        type=parse_icon_spec,
        metavar="NAME=PATH",
        help="Optional source image for a package icon",
    )
    args = parser.parse_args()

    repo_dir = args.repo_dir.expanduser().resolve()
    repo_dir.mkdir(parents=True, exist_ok=True)

    packages = load_existing_packages(repo_dir)
    packages_by_id = {}
    for package in packages:
        packages_by_id[package_identity(package)] = package
    prefix = args.artifact_prefix.rstrip("/")
    descriptions = dict(args.package_description)
    categories = dict(args.package_category)
    icons = dict(args.package_icon)

    for spec in args.package:
        relpath = artifact_relpath(args.arch, args.chip, args.compat, spec)
        destination = repo_dir / relpath
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(spec.source, destination)

        artifact = relpath.as_posix()
        if prefix:
            artifact = f"{prefix}/{artifact}"

        package = {
            "name": spec.name,
            "version": spec.version,
            "arch": args.arch,
            "compat": args.compat,
            "artifact": artifact,
            "sha256": sha256_file(destination),
            "type": spec.payload_type,
        }

        if spec.name in descriptions:
            package["description"] = descriptions[spec.name]

        if spec.name in categories:
            package["category"] = categories[spec.name]

        if spec.name in icons:
            icon_bytes = encode_icon_rgb565(icons[spec.name])
            icon_dest = repo_dir / icon_relpath(
                args.arch, args.chip, args.compat, spec.name
            )
            icon_dest.parent.mkdir(parents=True, exist_ok=True)
            icon_dest.write_bytes(icon_bytes)

            icon_rel = icon_relpath(
                args.arch, args.chip, args.compat, spec.name
            ).as_posix()
            package["icon"] = f"{prefix}/{icon_rel}" if prefix else icon_rel

        packages_by_id[package_identity(package)] = package

    packages = sorted(
        packages_by_id.values(),
        key=lambda item: (
            item["name"],
            item["version"],
            item["arch"],
            item["compat"],
            item["type"],
        ),
    )
    emit_index(repo_dir, packages)

    print(f"exported {len(packages)} package(s) to {repo_dir}")
    for package in packages:
        print(
            f"- {package['name']} {package['version']} "
            f"[{package['type']}] -> {package['artifact']}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
