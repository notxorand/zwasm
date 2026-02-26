# zwasm Reliability Improvement — Plan

> Updated: 2026-02-26
> Principles & branch strategy: `@./.claude/rules/reliability-work.md`
> Progress: `@./.dev/reliability-handover.md`

## Goal

Make zwasm **undeniably correct and fast** on Mac (aarch64) and Ubuntu (x86_64).
zwasm philosophy: **100% spec compliance, runs everything wasmtime runs, lightweight yet wasmtime-competitive speed.**

## Priority Order

| Priority | Meaning | Criteria |
|----------|---------|----------|
| **A** | Correctness | spec/test/real-world fully working on arm64+amd64 |
| **B** | Feature completeness | Implement missing features (GC JIT, etc.) |
| **C** | Performance | Target wasmtime 1x, accept 1.5x, allow 2-3x for single-pass limits |

## Completed Phases

| Phase | Content | Status |
|-------|---------|--------|
| A-F | Environment/compilation/compat/E2E/bench/analysis | Done |
| G | Ubuntu cross-platform | Done — spec 62,158 (100%) |
| I | E2E 100% + FP correctness | Done — 792/792 |
| J | x86_64 JIT bug fixes | Done |
| K.old | JIT opcode coverage, self-call, div-const | Done |

## Active: Plan A — Incremental regression fix + feature implementation

### Phase 1: rw_c_string hang fix (Priority A — Correctness)

**Symptom**: zwasm hangs on rw_c_string (60s timeout). wasmtime runs in 9.3ms.
**Cause**: Introduced at ee5f585 (OSR). Worked fine at 22859e2 (21ms).
**Approach**: Investigate OSR back-edge detection or guard function misjudgment.

Verification:
- `./zig-out/bin/zwasm run test/realworld/wasm/c_string_processing.wasm` completes normally
- `zig build test` pass, spec pass, no benchmark regression
- **Record**: `bash bench/record.sh --id=P1 --reason="Fix rw_c_string hang"`

### Phase 2: nbody FP cache fix (Priority C — Regression)

**Symptom**: nbody 43.8ms (1.99x wasmtime). Was 8-12ms (0.5x) before be466a0.
**Cause**: be466a0 "Fix JIT FP precision: getOrLoad must check dirty FP cache first"
  — correctness fix is valid, but implementation over-evicts FP cache.
**Approach**: Restrict eviction to rd==rs1 case only. Maintain correctness.
**Target**: Restore to 10-15ms (≤0.7x wasmtime).

Verification:
- nbody ≤ 15ms, spec pass, no regression on other benchmarks
- **Record**: `bash bench/record.sh --id=P2 --reason="Fix nbody FP cache regression"`

### Phase 3: rw_c_math re-measure (Priority C) — ACCEPTED AS EXCEPTION

**Symptom**: 58ms (4.92x wasmtime 11.8ms). Previous 16.4ms was anomalous measurement.
**Root cause**: c_math_compute has a single hot function (func#5) with 1381 IR instrs,
  136 vregs, 36 locals. Single-pass regalloc produces 876 STRs + 426 LDRs + 265 FMOVs
  out of 3323 total ARM64 instructions (38% memory traffic). wasmtime uses graph-coloring
  regalloc2 which handles 136 vregs efficiently.
**Decision**: Accept as single-pass regalloc limitation (like st_matrix).
  No further optimization feasible without multi-pass register allocator.

Verification:
- **Record**: `bash bench/record.sh --id=P3 --reason="Re-measure: accept as regalloc limit"`

### Phase 4: GC JIT basic implementation (Priority B — Feature)

**Symptom**: gc_alloc 1.79x, gc_tree 4.40x. GC opcodes fall back to interpreter.
**Approach**: JIT-compile struct.new, struct.get, struct.set, array.new, array.get, array.set.
  GC collection logic does not affect JIT codegen — just emit load/store for
  struct/array memory layout.
**Target**: gc_alloc ≤1.5x, gc_tree ≤2x.

Verification:
- GC spec tests pass, unit tests pass
- **Record**: `bash bench/record.sh --id=P4 --reason="GC JIT basic opcodes"`

### Phase 5: st_matrix — accept as exception (Priority C — Single-pass limit)

**Symptom**: 296ms (3.23x wasmtime 92ms). 35 vregs, fundamental single-pass regalloc limit.
  cranelift uses graph-coloring regalloc for optimal spill placement.
**Decision**: Accept ≤3.5x. Try LRU eviction improvements if feasible,
  but 1.5x is not realistic for single-pass.
**Official exception**: Phase H Gate condition 6 exempts st_matrix.

---

## Phase H Gate — Entry Criteria

**Phase H may NOT begin until ALL of the following are satisfied.**

| # | Condition | Verification |
|---|-----------|-------------|
| 1 | E2E: **778/778 (100%)** | Mac: e2e runner 0 failures |
| 2 | Real-world Mac: **all PASS** | `bash test/realworld/run_compat.sh` exits 0 |
| 3 | Real-world Ubuntu: **all PASS with JIT** | SSH same |
| 4 | Spec Mac: **62,158/62,158** | `python3 test/spec/run_spec.py --build --summary` |
| 5 | Spec Ubuntu: **62,158/62,158** | SSH same |
| 6 | Benchmarks Mac: **≤1.5x wasmtime** | `bash bench/compare_runtimes.sh` |
|   | Exception: st_matrix ≤3.5x (single-pass regalloc limit) | |
| 7 | Benchmarks Ubuntu: **≤1.5x wasmtime** (same exception) | SSH same |
| 8 | Unit tests: **Mac + Ubuntu PASS** | `zig build test` |
| 9 | Benchmark regression: **none vs history.yaml** | `bash bench/run_bench.sh` |

---

## Phase H: Documentation Accuracy (LAST)

Begins only after Phase H Gate passes. README claims audit, benchmark table update.
