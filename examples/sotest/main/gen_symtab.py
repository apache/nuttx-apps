#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0

import pathlib
import subprocess
import sys


def usage() -> int:
    print(
        "usage: gen_symtab.py <input> [<input> ...] <prefix> <output>",
        file=sys.stderr,
    )
    return 1


def run_nm(path: pathlib.Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["nm", str(path)],
        check=False,
        text=True,
        capture_output=True,
    )


def symbol_name(line: str) -> str:
    return line.split()[-1].replace('"', "")


def collect_symbols(paths):
    undefined: set[str] = set()
    defined: set[str] = set()

    for path in paths:
        result = run_nm(path)
        if result.returncode not in (0, 1):
            raise RuntimeError(result.stderr.strip() or f"nm failed for {path}")

        for line in result.stdout.splitlines():
            if " U " in line:
                undefined.add(symbol_name(line))
            elif line and not line.endswith(":"):
                defined.add(symbol_name(line))

    return sorted(undefined - defined)


def write_output(path: pathlib.Path, prefix: str, symbols) -> None:
    lines = [
        "#include <nuttx/compiler.h>",
        "#include <nuttx/symtab.h>",
        "",
    ]

    for symbol in symbols:
        lines.append(f"extern void *{symbol};")

    lines.append("")
    lines.append(f"const struct symtab_s {prefix}_exports[] = ")
    lines.append("{")

    for symbol in symbols:
        lines.append(f'  {{"{symbol}", &{symbol}}},')

    lines.append("};")
    lines.append("")
    lines.append(
        f"const int {prefix}_nexports = sizeof({prefix}_exports) / sizeof(struct symtab_s);"
    )
    lines.append("")

    text = "\n".join(lines)
    if path.exists() and path.read_text(encoding="utf-8") == text:
        return

    path.write_text(text, encoding="utf-8")


def main() -> int:
    if len(sys.argv) < 4:
        return usage()

    *input_paths, prefix, output = sys.argv[1:]
    symbols = collect_symbols([pathlib.Path(item) for item in input_paths])
    write_output(pathlib.Path(output), prefix, symbols)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
