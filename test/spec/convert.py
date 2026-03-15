#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
OUTDIR = ROOT / "test" / "spec" / "json"
SKIP_RE = re.compile(r"memory64|address64|align64|float_memory64|binary_leb128_64")
SKIP_VARIANT_RE = re.compile(r"^(address|align|binary)[0-9]$")


def convert_wast(wast: Path, outname: str) -> bool:
    result = subprocess.run(
        [
            "wasm-tools",
            "json-from-wast",
            str(wast),
            "-o",
            str(OUTDIR / f"{outname}.json"),
            "--wasm-dir",
            str(OUTDIR),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return result.returncode == 0


def iter_wast_files(directory: Path) -> list[Path]:
    return sorted(directory.glob("*.wast"))


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert WebAssembly spec .wast files to JSON.")
    parser.add_argument("testsuite", nargs="?", default="test/spec/testsuite")
    args = parser.parse_args()

    testsuite = Path(args.testsuite)
    if not testsuite.is_absolute():
        testsuite = ROOT / testsuite

    if not testsuite.is_dir() or not iter_wast_files(testsuite):
        print(f"Testsuite not found at {testsuite}")
        print("Run: git submodule update --init")
        return 1

    if shutil.which("wasm-tools") is None:
        print("Error: wasm-tools not found. Install: cargo install wasm-tools")
        return 1

    OUTDIR.mkdir(parents=True, exist_ok=True)

    converted = 0
    skipped = 0
    failed = 0

    for wast in iter_wast_files(testsuite):
        name = wast.stem
        if SKIP_RE.search(name) or SKIP_VARIANT_RE.search(name):
            skipped += 1
            continue
        if convert_wast(wast, name):
            converted += 1
        else:
            print(f"WARN: failed to convert {name}.wast")
            failed += 1

    for subdir in ("multi-memory", "relaxed-simd"):
        sub_path = testsuite / subdir
        if not sub_path.is_dir():
            continue
        for wast in iter_wast_files(sub_path):
            if convert_wast(wast, wast.stem):
                converted += 1
            else:
                print(f"WARN: failed to convert {wast.name}")
                failed += 1

    gc_root = Path(os.environ.get("GC_TESTSUITE", str(Path.home() / "Documents" / "OSS" / "WebAssembly" / "gc")))
    tsi = gc_root / "test" / "core" / "gc" / "type-subtyping-invalid.wast"
    if tsi.is_file():
        if convert_wast(tsi, "type-subtyping-invalid"):
            converted += 1
        else:
            print("WARN: failed to convert type-subtyping-invalid.wast")
            failed += 1

    print()
    print(f"Converted: {converted}, Skipped: {skipped}, Failed: {failed}")
    print(f"Output: {OUTDIR}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
