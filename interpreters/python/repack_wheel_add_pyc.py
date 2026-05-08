#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
#
# Repack a wheel in-place: compile pip/*.py to legacy sibling *.pyc (compileall
# -b: required for zipimport, which does not read PEP 3147 __pycache__/ names),
# remove the .py sources, and rewrite *.dist-info/RECORD.

from __future__ import annotations

import argparse
import base64
import hashlib
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


def wheel_record_hash(data: bytes) -> str:
    digest = hashlib.sha256(data).digest()
    return base64.urlsafe_b64encode(digest).decode("ascii").rstrip("=")


def wheel_has_pip_py_sources(zf: zipfile.ZipFile) -> bool:
    return any(n.startswith("pip/") and n.endswith(".py") for n in zf.namelist())


def wheel_has_legacy_pip_bytecode(zf: zipfile.ZipFile) -> bool:
    return "pip/__init__.pyc" in zf.namelist()


def strip_pip_py_sources(pip_dir: Path) -> int:
    """Remove pip/**/*.py after sibling legacy *.pyc exists (compileall -b output)."""
    removed = 0
    for path in sorted(pip_dir.rglob("*.py")):
        if not path.is_file():
            continue
        pyc = path.with_suffix(".pyc")
        if not pyc.is_file():
            rel = path.relative_to(pip_dir)
            raise SystemExit(
                f"missing legacy .pyc for pip/{rel.as_posix()}, refusing to delete source"
            )
        path.unlink()
        removed += 1
    return removed


def rebuild_record(root: Path) -> None:
    dist_infos = sorted(root.glob("*.dist-info"))
    if len(dist_infos) != 1:
        raise SystemExit(
            f"expected one *.dist-info, got {[p.name for p in dist_infos]}"
        )
    di = dist_infos[0]
    record_path = di / "RECORD"
    record_rel = f"{di.name}/RECORD"
    lines: list[str] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(root).as_posix()
        if rel == record_rel:
            continue
        body = path.read_bytes()
        lines.append(f"{rel},sha256={wheel_record_hash(body)},{len(body)}")
    lines.append(f"{record_rel},,")
    record_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def repack(whl_path: Path, *, force: bool) -> None:
    whl_path = whl_path.resolve()
    if not whl_path.is_file():
        raise SystemExit(f"missing wheel: {whl_path}")

    with zipfile.ZipFile(whl_path) as zf:
        has_py = wheel_has_pip_py_sources(zf)
        if not has_py:
            if not wheel_has_legacy_pip_bytecode(zf):
                raise SystemExit(
                    "repack_wheel_add_pyc: wheel has no pip/*.py and no pip/__init__.pyc "
                    "(corrupt or old tool output). Delete the bundled pip-*.whl and rebuild."
                )
            if not force:
                print(
                    f"repack_wheel_add_pyc: skip (pip already bytecode-only): {whl_path.name}"
                )
                return

    tmpdir = tempfile.mkdtemp(prefix="pip-whl-pyc-")
    try:
        root = Path(tmpdir)
        with zipfile.ZipFile(whl_path) as zf:
            zf.extractall(root)

        pip_dir = root / "pip"
        if not pip_dir.is_dir():
            raise SystemExit("wheel has no pip/ top-level package")

        subprocess.run(
            [sys.executable, "-m", "compileall", "-q", "-f", "-b", str(pip_dir)],
            cwd=str(root),
            check=True,
        )
        n_py = strip_pip_py_sources(pip_dir)
        rebuild_record(root)

        out_path = whl_path.with_suffix(whl_path.suffix + ".tmp")
        with zipfile.ZipFile(out_path, "w", compression=zipfile.ZIP_DEFLATED) as out:
            for path in sorted(root.rglob("*")):
                if path.is_file():
                    arcname = path.relative_to(root).as_posix()
                    out.write(path, arcname)

        out_path.replace(whl_path)
        print(
            f"repack_wheel_add_pyc: bytecode-only pip ({n_py} .py removed) -> {whl_path.name}"
        )
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("wheel", type=Path, help="path to .whl (updated in place)")
    ap.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="repack even if pip is already .pyc-only",
    )
    args = ap.parse_args()
    repack(args.wheel, force=args.force)


if __name__ == "__main__":
    main()
