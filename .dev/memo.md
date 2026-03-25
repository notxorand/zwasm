# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 + Phase 1, 3, 5, 8, 10, 11, 13, 15, 19, **20** complete.
- Spec: 62,263/62,263 Mac+Ubuntu (100.0%, 0 skip).
- E2E: 797/797 (Mac+Ubuntu). Fixed JIT memory64 bounds + custom-page-sizes 2026-03-25.
- Real-world: Mac 50/50, Ubuntu 50/50. go_math_big fixed 2026-03-25.
- JIT: Register IR + ARM64/x86_64 + SIMD (NEON 253/256, SSE 244/256).
- HOT_THRESHOLD=3 (lowered from 10 in W38).
- Binary: 1.29MB stripped. Memory: ~3.5MB RSS.
- Platforms: macOS ARM64, Linux x86_64/ARM64, Windows x86_64.
- **main = stable**. ClojureWasm updated to v1.5.0.

## Current Task

**W45: SIMD Loop Persistence — NEXT**

Keep Q/XMM-cached v128 values alive across loop iterations. Currently
`evictAllCaches()` at every branch target (including loop headers) flushes
all Q regs, destroying the W44 cache benefit in loops.

### Problem (diagnosed 2026-03-26)

```
loop_header:          ← evictAllCaches() fires here
  STR Q16 → mem       ; flush (5-6 vregs per iter)
  STR Q17 → mem
  LDR Q16 ← mem       ; reload (cold)
  FMUL V17, V16, V16  ; actual compute
  ...
  B loop_header        ; → flush again
```

Wasmtime keeps v128 in regs across iterations — 0 memory traffic.

### Facts

- ARM64 native SIMD: 247/276 ops (89%) — trampoline NOT the bottleneck
- Mandelbrot: 12 NEON + 12 loads + 12 stores/iter → should be 12 NEON only
- zwasm SIMD is **slower** than scalar (eviction overhead > SIMD benefit)
- Current SIMD gap: 38-248x vs wasmtime (scalar gap: 1-6x)

### Baseline benchmarks (2026-03-26, zwasm JIT cached vs wasmtime AOT cached)

| Benchmark            | zwasm  | wasmtime | ratio  | notes          |
|----------------------|--------|----------|--------|----------------|
| simd_mandel (simd)   | 18.7s  | 240ms    | 78x    | loop-dominated |
| simd_matmul (simd)   | 2.7s   | 20ms     | 136x   | loop-dominated |
| simd_chain           | 390ms  | 10ms     | 39x    | loop-dominated |
| simd_nbody (simd)    | 520ms  | 10ms     | 52x    | loop-dominated |
| fib (cached)         | 50ms   | 80ms     | 0.6x   | zwasm wins     |
| st_fib2 (cached)     | 900ms  | 680ms    | 1.3x   | healthy        |
| st_matrix (cached)   | 330ms  | 90ms     | 3.7x   | scalar gap     |

### Fix path (ordered by impact)

1. **Loop-header Q-reg persistence** → 2-3x improvement
   - Distinguish loop backedge targets from merge points
   - At loop headers: skip eviction (same Q state from backedge)
   - At merge points (if/else join): still evict (different paths)
   - Key: `scanBranchTargets` needs loop detection (back edges)
   - Code: `jit.zig` compile loop (line ~2604), `x86.zig` (line ~3872)
   - Reference: fp_dreg cache already skips eviction in some cases

2. **v128.load/store bounds check elimination** → 1.5-2x
   - Guard pages already exist (`use_guard_pages`)
   - v128.load/store should use same guard page path as scalar loads

3. **FMLA instruction fusion** → 1.2-1.5x
   - Detect `f32x4.mul + f32x4.add` pattern → emit ARM64 FMLA
   - Peephole in emitSimdNativeInner or IR fusion pass

4. Realistic target: **10-20x of wasmtime** (from current 38-248x)

### Key code locations

- `jit.zig:2604`: branch target eviction (ARM64)
- `x86.zig:3872`: branch target eviction (x86)
- `jit.zig:simdQregEvictAll`: Q-cache eviction function
- `jit.zig:emitSimdBinaryNeon`: direct Q-reg binary ops (already optimal within basic block)
- `jit.zig:scanBranchTargets`: identifies branch targets (needs loop detection)

### SIMD benchmarks

Build: `bash bench/simd/build_simd_bench.sh`
Compare: `bash bench/compare_runtimes.sh --rt=zwasm,wasmtime`
Sources: `bench/simd/src/` (C: mandelbrot, matmul, simd_chain, nbody, etc.)
         `bench/simd/rust-blake2/` (Rust: blake2b_simd)

### Open Work Items

| Item       | Description                                       | Status           |
|------------|---------------------------------------------------|------------------|
| **W45**    | **SIMD loop persistence (Q-reg across loops)**    | **Next**         |
| W44        | SIMD register class (D132 Phase B)                | DONE (2026-3-26) |
| Phase 18   | Lazy Compilation + CLI Extensions                 | Future           |
| Zig 0.16   | API breaking changes                              | When released    |

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
2. **This memo**: current task (W44 design + implementation plan)
3. **D132 Phase B**: `@./.dev/decisions.md` → search `## D132` → "Phase B"
4. **regalloc.zig**: `RegInstr` struct, SIMD opcode handling (search `SIMD_BASE`)
5. **jit.zig**: `emitSimdBinaryOp`, `emitLoadV128`, `emitStoreV128`, `SIMD_SCRATCH0/1`
6. **x86.zig**: same patterns as jit.zig
7. **Reference impl**: wasmtime cranelift (`~/Documents/OSS/wasmtime/`) — SIMD register allocation
8. **SIMD benchmarks**: `bench/run_simd_bench.sh` — A/B comparison
9. **Ubuntu testing**: `@./.dev/references/ubuntu-testing-guide.md`
10. **Merge gate**: CLAUDE.md → "Merge Gate Checklist" section

## References

- `@./.dev/roadmap.md` — Phase roadmap
- `@./.dev/checklist.md` — resolved work items
- `@./.dev/decisions.md` — D130 (SIMD arch), D132 (SIMD perf plan)
- `@./.dev/jit-debugging.md` — JIT debug techniques
- `@./.dev/references/w38-osr-research.md` — OSR research
- `bench/simd_comparison.yaml` — SIMD performance data
- `bench/history.yaml` — benchmark history (latest: phase20-rem-fix)
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
