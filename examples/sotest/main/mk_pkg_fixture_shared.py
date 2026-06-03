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


def write_shared_index(
    path: pathlib.Path,
    arch: str,
    compat: str,
    modprint_digest: str,
    sotest_digest: str,
) -> None:
    payload = {
        "packages": [
            {
                "name": "modprint",
                "version": "1.0.0",
                "arch": arch,
                "compat": compat,
                "artifact": "/mnt/sotest/romfs/modprint",
                "sha256": modprint_digest,
                "type": "shared-lib",
            },
            {
                "name": "modprint",
                "version": "9.9.9",
                "arch": "arm",
                "compat": "stm32f4discovery",
                "artifact": "/mnt/sotest/romfs/modprint",
                "sha256": modprint_digest,
                "type": "shared-lib",
            },
            {
                "name": "sotest",
                "version": "1.0.0",
                "arch": arch,
                "compat": compat,
                "artifact": "/mnt/sotest/romfs/sotest",
                "sha256": sotest_digest,
                "type": "shared-lib",
            },
            {
                "name": "sotest",
                "version": "9.9.9",
                "arch": "arm",
                "compat": "stm32f4discovery",
                "artifact": "/mnt/sotest/romfs/sotest",
                "sha256": sotest_digest,
                "type": "shared-lib",
            },
        ]
    }

    path.write_text(json.dumps(payload, separators=(",", ":")) + "\n", encoding="utf-8")


def write_shared_script(path: pathlib.Path) -> None:
    script = "\n".join(
        [
            "mount -t tmpfs /etc",
            "mount -t tmpfs /var",
            "mkdir /etc/nxpkg",
            "cp /mnt/sotest/romfs/shared-index.json /etc/nxpkg/index.json",
            "nxpkg install modprint",
            "nxpkg install sotest",
            "cp /var/lib/nxpkg/pkgs/modprint/1.0.0/modprint /etc/m",
            "cp /var/lib/nxpkg/pkgs/sotest/1.0.0/sotest /etc/s",
            "sotest /etc/m /etc/s",
            "nxpkg list",
            "",
        ]
    )
    path.write_text(script, encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 7:
        print(
            "usage: mk_pkg_fixture_shared.py <modprint-bin> <sotest-bin> "
            "<shared-index-json> <pkgsotest-nsh> <arch> <compat>",
            file=sys.stderr,
        )
        return 1

    modprint = pathlib.Path(sys.argv[1])
    sotest = pathlib.Path(sys.argv[2])
    shared_index = pathlib.Path(sys.argv[3])
    shared_script = pathlib.Path(sys.argv[4])
    arch = sys.argv[5]
    compat = sys.argv[6]

    write_shared_index(
        shared_index,
        arch,
        compat,
        sha256_file(modprint),
        sha256_file(sotest),
    )
    write_shared_script(shared_script)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
