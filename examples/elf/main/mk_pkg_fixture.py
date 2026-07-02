#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0

import hashlib
import json
import pathlib
import sys


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as infile:
        while True:
            chunk = infile.read(4096)
            if not chunk:
                break
            digest.update(chunk)

    return digest.hexdigest()


def write_index(
    path: pathlib.Path,
    arch: str,
    compat: str,
    artifact: str,
    digest: str,
) -> None:
    payload = {
        "packages": [
            {
                "name": "hello",
                "version": "1.0.0",
                "arch": arch,
                "compat": compat,
                "artifact": artifact,
                "sha256": digest,
                "type": "elf",
            },
            {
                "name": "hello",
                "version": "9.9.9",
                "arch": "arm",
                "compat": "stm32f4discovery",
                "artifact": artifact,
                "sha256": digest,
                "type": "elf",
            },
        ]
    }

    path.write_text(json.dumps(payload, separators=(",", ":")) + "\n", encoding="utf-8")


def write_bad_index(path: pathlib.Path, arch: str, compat: str) -> None:
    payload = {
        "packages": [
            {
                "name": "hello-missing",
                "version": "1.0.0",
                "arch": arch,
                "compat": compat,
                "artifact": "/mnt/elf/romfs/hello-missing",
                "sha256": "0" * 64,
                "type": "elf",
            }
        ]
    }

    path.write_text(json.dumps(payload, separators=(",", ":")) + "\n", encoding="utf-8")


def write_script(path: pathlib.Path) -> None:
    script = "\n".join(
        [
            "mount -t tmpfs /etc",
            "mount -t tmpfs /var",
            "mkdir /etc/nxpkg",
            "cp /mnt/elf/romfs/index.json /etc/nxpkg/index.json",
            "nxpkg install hello",
            "nxpkg list",
            "",
        ]
    )
    path.write_text(script, encoding="utf-8")


def write_fail_script(path: pathlib.Path) -> None:
    script = "\n".join(
        [
            "mount -t tmpfs /etc",
            "mount -t tmpfs /var",
            "mkdir /etc/nxpkg",
            "cp /mnt/elf/romfs/bad-index.json /etc/nxpkg/index.json",
            "nxpkg install hello-missing",
            "",
        ]
    )
    path.write_text(script, encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 8:
        print(
            "usage: mk_pkg_fixture.py <hello-bin> <index-json> <bad-index-json> "
            "<pkgtest-nsh> <pkgfail-nsh> <arch> <compat>",
            file=sys.stderr,
        )
        return 1

    hello = pathlib.Path(sys.argv[1])
    index = pathlib.Path(sys.argv[2])
    bad_index = pathlib.Path(sys.argv[3])
    script = pathlib.Path(sys.argv[4])
    fail_script = pathlib.Path(sys.argv[5])
    arch = sys.argv[6]
    compat = sys.argv[7]
    artifact = "/mnt/elf/romfs/hello"

    digest = sha256_file(hello)
    write_index(index, arch, compat, artifact, digest)
    write_bad_index(bad_index, arch, compat)
    write_script(script)
    write_fail_script(fail_script)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
