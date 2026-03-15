#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
WAST_DIR = ROOT / "test" / "e2e" / "wast"
JSON_DIR = ROOT / "test" / "e2e" / "json"
SKIP_FILE = ROOT / "test" / "e2e" / "skip.txt"

BATCH1 = [
    "add.wast", "div-rem.wast", "mul16-negative.wast", "wide-arithmetic.wast",
    "control-flow.wast", "br-table-fuzzbug.wast", "simple-unreachable.wast",
    "misc_traps.wast", "stack_overflow.wast", "no-panic.wast", "no-panic-on-invalid.wast",
    "memory-copy.wast", "imported-memory-copy.wast", "partial-init-memory-segment.wast",
    "call_indirect.wast", "many-results.wast", "many-return-values.wast",
    "export-large-signature.wast", "func-400-params.wast", "table_copy.wast",
    "table_copy_on_imported_tables.wast", "elem_drop.wast", "elem-ref-null.wast",
    "table_grow_with_funcref.wast", "linking-errors.wast", "empty.wast",
]

BATCH2 = [
    "f64-copysign.wast", "float-round-doesnt-load-too-much.wast",
    "int-to-float-splat.wast", "sink-float-but-dont-trap.wast",
    "externref-id-function.wast", "externref-segment.wast",
    "mutable_externref_globals.wast", "simple_ref_is_null.wast",
    "externref-table-dropped-segment-issue-8281.wast", "bit-and-conditions.wast",
    "no-opt-panic-dividing-by-zero.wast", "partial-init-table-segment.wast",
    "many_table_gets_lead_to_gc.wast", "no-mixup-stack-maps.wast", "rs2wasm-add-func.wast",
]

BATCH3 = [
    "embenchen_fannkuch.wast", "embenchen_fasta.wast", "embenchen_ifs.wast",
    "embenchen_primes.wast", "rust_fannkuch.wast", "fib.wast", "issue1809.wast",
    "issue4840.wast", "issue4857.wast", "issue4890.wast", "issue6562.wast",
    "issue694.wast", "issue11748.wast", "issue12318.wast",
]

BATCH3_WAT: list[str] = []

BATCH4_SIMD = [
    "simd/cvt-from-uint.wast", "simd/edge-of-memory.wast", "simd/unaligned-load.wast",
    "simd/load_splat_out_of_bounds.wast", "simd/v128-select.wast",
    "simd/replace-lane-preserve.wast", "simd/almost-extmul.wast",
    "simd/interesting-float-splat.wast", "simd/issue4807.wast",
    "simd/issue6725-no-egraph-panic.wast", "simd/issue_3173_select_v128.wast",
    "simd/issue_3327_bnot_lowering.wast", "simd/spillslot-size-fuzzbug.wast",
    "simd/sse-cannot-fold-unaligned-loads.wast",
]

BATCH5_PROPOSALS = [
    "function-references/call_indirect.wast", "function-references/table_fill.wast",
    "function-references/table_get.wast", "function-references/table_grow.wast",
    "function-references/table_set.wast", "tail-call/loop-across-modules.wast",
    "multi-memory/simple.wast", "threads/LB.wast", "threads/LB_atomic.wast",
    "threads/MP.wast", "threads/MP_atomic.wast", "threads/MP_wait.wast",
    "threads/SB.wast", "threads/SB_atomic.wast", "threads/atomics-end-of-memory.wast",
    "threads/atomics_notify.wast", "threads/atomics_wait_address.wast",
    "threads/load-store-alignment.wast", "threads/wait_notify.wast",
    "memory64/bounds.wast", "memory64/codegen.wast", "memory64/linking-errors.wast",
    "memory64/linking.wast", "memory64/multi-memory.wast", "memory64/offsets.wast",
    "memory64/simd.wast", "memory64/threads.wast", "gc/alloc-v128-struct.wast",
    "gc/anyref_that_is_i31_barriers.wast", "gc/array-alloc-too-large.wast",
    "gc/array-init-data.wast", "gc/array-new-data.wast", "gc/array-new-elem.wast",
    "gc/array-types.wast", "gc/arrays-of-different-types.wast",
    "gc/externrefs-can-be-i31refs.wast", "gc/func-refs-in-gc-heap.wast",
    "gc/fuzz-segfault.wast", "gc/i31ref-of-global-initializers.wast",
    "gc/i31ref-tables.wast", "gc/issue-10171.wast", "gc/issue-10182.wast",
    "gc/issue-10353.wast", "gc/issue-10397.wast", "gc/issue-10459.wast",
    "gc/issue-10467.wast", "gc/more-rec-groups-than-types.wast", "gc/null-i31ref.wast",
    "gc/rec-group-funcs.wast", "gc/ref-test.wast", "gc/struct-instructions.wast",
    "gc/struct-types.wast",
]


def parse_skip_file() -> tuple[list[str], list[str]]:
    skip_dirs: list[str] = []
    skip_files: list[str] = []
    for raw in SKIP_FILE.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].rstrip()
        if not line:
            continue
        if line.endswith("/"):
            skip_dirs.append(line)
        else:
            skip_files.append(line)
    return skip_dirs, skip_files


def is_skipped(src: Path, root: Path, skip_dirs: list[str], skip_files: list[str]) -> bool:
    rel = src.relative_to(root).as_posix()
    if any(rel.startswith(prefix) for prefix in skip_dirs):
        return True
    return src.name in skip_files


def flatten_name(src: Path, root: Path) -> tuple[str, str]:
    rel = src.relative_to(root).as_posix()
    name = src.name
    base = src.stem
    if "/" not in rel:
        return name, base
    prefix = rel.split("/", 1)[0].replace("-", "_")
    if prefix == "function_references":
        prefix = "funcref"
    return f"{prefix}_{name}", f"{prefix}_{base}"


def selected_files(batch: str | None, root: Path) -> list[Path]:
    mapping = {
        "1": BATCH1,
        "2": BATCH2,
        "3": BATCH3 + BATCH3_WAT,
        "4": BATCH4_SIMD,
        "5": BATCH5_PROPOSALS,
    }
    rels = mapping.get(batch, BATCH1 + BATCH2 + BATCH3 + BATCH3_WAT + BATCH4_SIMD + BATCH5_PROPOSALS)
    return [root / rel for rel in rels]


def convert_with_wasm_tools(src: Path, out_json: Path) -> bool:
    result = subprocess.run(
        ["wasm-tools", "json-from-wast", str(src), "-o", str(out_json), "--wasm-dir", str(JSON_DIR)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return result.returncode == 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Copy and convert wasmtime misc_testsuite files.")
    parser.add_argument("--batch")
    args = parser.parse_args()

    wasmtime_misc = Path(os.environ.get("WASMTIME_MISC_DIR", str(Path.home() / "Documents" / "OSS" / "wasmtime" / "tests" / "misc_testsuite")))
    if not wasmtime_misc.is_dir():
        print(f"ERROR: wasmtime misc_testsuite not found at {wasmtime_misc}")
        return 1

    if shutil.which("wasm-tools") is None:
        print("ERROR: wasm-tools not found")
        return 1

    WAST_DIR.mkdir(parents=True, exist_ok=True)
    JSON_DIR.mkdir(parents=True, exist_ok=True)

    skip_dirs, skip_files = parse_skip_file()
    converted = skipped = failed = copied = 0

    for src in selected_files(args.batch, wasmtime_misc):
        if not src.is_file():
            continue
        if is_skipped(src, wasmtime_misc, skip_dirs, skip_files):
            skipped += 1
            continue

        flat_name, flat_base = flatten_name(src, wasmtime_misc)
        dest = WAST_DIR / flat_name
        shutil.copy2(src, dest)
        copied += 1

        if src.suffix == ".wast":
            if convert_with_wasm_tools(dest, JSON_DIR / f"{flat_base}.json"):
                converted += 1
            else:
                print(f"WARN: failed to convert {flat_name}")
                failed += 1
        elif src.suffix == ".wat":
            print(f"NOTE: {flat_name} is .wat; skipping JSON conversion")
            skipped += 1

    print()
    print(f"Copied: {copied}, Converted: {converted}, Skipped: {skipped}, Failed: {failed}")
    print(f"WAST dir: {WAST_DIR}")
    print(f"JSON dir: {JSON_DIR}")

    print()
    print("--- Custom proposal generators ---")
    generators = []
    if (WAST_DIR / "wide-arithmetic.wast").is_file():
        generators.append(ROOT / "test" / "e2e" / "gen_wide_arithmetic.py")
    generators.append(ROOT / "test" / "e2e" / "gen_custom_page_sizes.py")
    for generator in generators:
        result = subprocess.run([sys.executable, str(generator)], check=False)
        name = generator.stem.replace("gen_", "").replace("_", "-")
        print(f"{'OK' if result.returncode == 0 else 'FAIL'}: {name}")
        if result.returncode != 0:
            failed += 1

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
