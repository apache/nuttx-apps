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
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List

VALID_TYPES = {"elf", "shared-lib"}
PACKAGE_SPEC_HELP = (
    "package must look like " "<name>:<version>:<elf|shared-lib>:<source>"
)


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


def artifact_relpath(arch: str, chip: str, compat: str,
                     spec: PackageSpec) -> Path:  # fmt: skip
    filename = spec.source.name
    path = Path("artifacts") / arch / chip / compat
    path = path / spec.name / spec.version / filename
    return path


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
    args = parser.parse_args()

    repo_dir = args.repo_dir.expanduser().resolve()
    repo_dir.mkdir(parents=True, exist_ok=True)

    packages = load_existing_packages(repo_dir)
    packages_by_id = {}
    for package in packages:
        packages_by_id[package_identity(package)] = package
    prefix = args.artifact_prefix.rstrip("/")

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
