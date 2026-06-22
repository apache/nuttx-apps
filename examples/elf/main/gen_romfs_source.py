#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0

import pathlib
import sys


def usage() -> int:
    print("usage: gen_romfs_source.py <input> <output>", file=sys.stderr)
    return 1


def symbol_from_path(path: pathlib.Path) -> str:
    return path.name.replace(".", "_")


def format_bytes(data: bytes) -> str:
    rows = []
    for index in range(0, len(data), 12):
        chunk = data[index : index + 12]
        rows.append("  " + ", ".join(f"0x{byte:02x}" for byte in chunk))
    return ",\n".join(rows)


def write_output(input_path: pathlib.Path, output_path: pathlib.Path) -> None:
    symbol = symbol_from_path(input_path)
    data = input_path.read_bytes()
    text = "\n".join(
        [
            "#include <nuttx/compiler.h>",
            "",
            f"const unsigned char aligned_data(4) {symbol}[] = {{",
            format_bytes(data),
            "};",
            f"const unsigned int {symbol}_len = {len(data)};",
            "",
        ]
    )

    if output_path.exists() and output_path.read_text(encoding="utf-8") == text:
        return

    output_path.write_text(text, encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 3:
        return usage()

    input_path = pathlib.Path(sys.argv[1])
    output_path = pathlib.Path(sys.argv[2])
    write_output(input_path, output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
