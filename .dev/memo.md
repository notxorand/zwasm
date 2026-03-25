# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 + Phase 1, 3, 5, 8, 10, 11, 13, 15, 19, **20** complete.
- Spec: 62,263/62,263 Mac+Ubuntu (100.0%, 0 skip).
- E2E: 792/792 (Mac+Ubuntu).
- Real-world: Mac 50/50, Ubuntu 50/50. go_math_big fixed 2026-03-25.
- JIT: Register IR + ARM64/x86_64 + SIMD (NEON 253/256, SSE 244/256).
- HOT_THRESHOLD=3 (lowered from 10 in W38).
- Binary: 1.29MB stripped. Memory: ~3.5MB RSS.
- Platforms: macOS ARM64, Linux x86_64/ARM64, Windows x86_64.
- **main = stable**. ClojureWasm updated to v1.5.0.

## Current Task

**W43: SIMD v128 base address cache (D132 Phase A) — DONE**

ARM64: Cache `vm_ptr + simd_v128_offset` in x17 (SIMD_BASE_REG) when `has_simd`.
Reduces v128 address computation from 3-4 instructions to 1-2 per SIMD op.
x86 skipped — only 1 insn saving (imm32 native), not worth losing a vreg.

Key changes in `src/jit.zig`:
- `simd_base_cached` flag, `SIMD_BASE_REG = 17` (x17)
- `vregToPhysEff` / `destRegEff` — SIMD-aware vreg-to-phys mapping
- `effectiveMaxRegs()` — 22 when cached (vreg 22 excluded)
- `emitLoadSimdBase()` — prologue, OSR prologue, and after all BLR calls
- Reload x17 in `reloadCallerSaved*`, fuel check stub

### Open Work Items

| Item       | Description                                       | Status         |
|------------|---------------------------------------------------|----------------|
| W44        | SIMD register class (D132 Phase B)                | Future         |
| Phase 18   | Lazy Compilation + CLI Extensions                 | Future         |
| Zig 0.16   | API breaking changes                              | When released  |

## Completed Phases (summary)

| Phase    | Name                                  | Date       |
|----------|---------------------------------------|------------|
| 1        | Guard Pages + Module Cache            | 2026-03    |
| 3        | CI Automation + Documentation         | 2026-03    |
| 5        | C API + Conditional Compilation       | 2026-03    |
| 8        | Real-World Coverage + WAT Parity      | 2026-03    |
| 10       | Quality / Stabilization               | 2026-03    |
| 11       | Allocator Injection + Embedding       | 2026-03    |
| 13       | SIMD JIT (NEON + SSE)                 | 2026-03-23 |
| 15       | Windows Port                          | 2026-03    |
| 19       | JIT Reliability                       | 2026-03    |
| 20       | JIT Correctness Sweep                 | 2026-03-25 |

## Next Session Reference Chain

1. **Orient**: `git log --oneline -5 && git status && git branch`
2. **This memo**: current task, open work items
3. **Roadmap**: `@./.dev/roadmap.md` — next priorities
4. **D132**: `@./.dev/decisions.md` → search `## D132` — SIMD two-phase plan
   - Phase A (W43): DONE — v128 base address cache in x17
   - Phase B (W44): SIMD register class — design challenges, deferred
5. **Ubuntu testing**: `@./.dev/references/ubuntu-testing-guide.md` — OrbStack VM
6. **Merge gate checklist**: CLAUDE.md → "Merge Gate Checklist" section

## References

- `@./.dev/roadmap.md` — Phase roadmap
- `@./.dev/checklist.md` — resolved work items
- `@./.dev/decisions.md` — D130 (SIMD arch), D132 (SIMD perf plan)
- `@./.dev/jit-debugging.md` — JIT debug techniques
- `@./.dev/references/w38-osr-research.md` — OSR research
- `bench/simd_comparison.yaml` — SIMD performance data
- `bench/history.yaml` — benchmark history (latest: phase20-rem-fix)
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
