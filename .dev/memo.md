# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 complete. v1.1.0 released. ~38K LOC, 510 unit tests.
- Spec: 62,158/62,158 Mac + Ubuntu (100.0%). E2E: 792/792 (100.0%).
- Wasm 3.0: all 9 proposals. WASI: 46/46 (100%). WAT parser complete.
- JIT: Register IR + ARM64/x86_64. Size: 1.31MB / 3.44MB RSS.
- **main = stable**: ClojureWasm depends on main (v1.1.0 tag).

## Current Task

Reliability improvement (branch: `strictly-check/reliability-004`).
P1+P2 merged to main. Now on P3-P5.
Plan: `@./.dev/reliability-plan.md`. Progress: `@./.dev/reliability-handover.md`.

**Plan A: Incremental regression fix + feature implementation**
- [x] P1: rw_c_string hang fix — skip back-edge JIT for reentry guard (20.2ms)
- [x] P2: nbody FP cache fix — expand D-reg cache D2-D15, FP-aware MOV (23.1ms, 0.97x wasmtime)
- [x] P3: rw_c_math — accepted as regalloc limit (58ms, 4.92x, 136 regs)
- P4: GC JIT basic implementation (Priority B)
- P5: st_matrix accept as exception (Priority C)

**Active: P4 (GC JIT basic implementation)**
JIT-compile struct.new, struct.get, struct.set, array.new, array.get, array.set.
Target: gc_alloc ≤1.5x, gc_tree ≤2x wasmtime.

## Previous Task

P2: Fix nbody FP cache regression — expanded D-reg cache from D2-D7 to D2-D15
using callee-saved registers, plus FP-cache-aware MOV. 43.8ms → 23.1ms (0.97x wasmtime).
register IR interpreter (20.2ms, was timeout).

## Known Bugs

- c_hello_wasi: EXIT=71 on Ubuntu (WASI issue, not JIT — same with --profile)
- Go WASI: 3 Go programs produce no output (WASI compatibility, not JIT-related)

## References

- `@./.dev/roadmap.md`, `@./private/roadmap-production.md` (stages)
- `@./.dev/decisions.md`, `@./.dev/checklist.md`, `@./.dev/spec-support.md`
- `@./.dev/reliability-plan.md` (plan), `@./.dev/reliability-handover.md` (progress)
- `@./.dev/jit-debugging.md`, `@./.dev/ubuntu-x86_64.md` (gitignored)
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
