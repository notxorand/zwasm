# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 complete. v1.1.0 released. ~38K LOC, 510 unit tests.
- Spec: 62,158/62,158 Mac + Ubuntu (100.0%). E2E: 792/792 (100.0%).
- Wasm 3.0: all 9 proposals. WASI: 46/46 (100%). WAT parser complete.
- JIT: Register IR + ARM64/x86_64. Size: 1.31MB / 3.44MB RSS.
- **main = stable**: ClojureWasm depends on main (v1.1.0 tag).

## Current Task

Reliability improvement (branch: `strictly-check/reliability-003`).
**P1+P2 完了まで main マージ禁止** (nbody regression + rw_c_string hang)。
Plan: `@./.dev/reliability-plan.md`. Progress: `@./.dev/reliability-handover.md`.

**Plan A: 段階的リグレッション修正 + 機能実装**
- P1: rw_c_string hang 修正 (Priority A — 正確性)
- P2: nbody FP キャッシュ修正 (Priority C — リグレッション)
- P3: rw_c_math 再計測 (Priority C)
- P4: GC JIT 基本実装 (Priority B)
- P5: st_matrix 許容判断 (Priority C)

**Active: P1 (rw_c_string hang)**
OSR (ee5f585) で発生。22859e2 時点では 21ms で正常。
back-edge 検出 or guard 判定の誤爆を調査。

## Previous Task

reliability-003 Phases A-K + OSR + bench infra upgrade:
- E2E 792/792, spec 62,158, x86 JIT fixes, self-call/div-const opt
- Bench recording upgraded: 29 benchmarks, runs=5/warmup=3, timeout
- history.yaml: per-commit rerun (28 commits)
- **発見**: be466a0 で nbody 4x リグレッション (FP cache precision fix)

## Known Bugs

- c_hello_wasi: EXIT=71 on Ubuntu (WASI issue, not JIT — same with --profile)
- Go WASI: 3 Go programs produce no output (WASI compatibility, not JIT-related)

## References

- `@./.dev/roadmap.md`, `@./private/roadmap-production.md` (stages)
- `@./.dev/decisions.md`, `@./.dev/checklist.md`, `@./.dev/spec-support.md`
- `@./.dev/reliability-plan.md` (plan), `@./.dev/reliability-handover.md` (progress)
- `@./.dev/jit-debugging.md`, `@./.dev/ubuntu-x86_64.md` (gitignored)
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
