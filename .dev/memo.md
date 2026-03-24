# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 + Phase 1, 3, 5, 8, 10, 11, 13, 15, 19 all complete.
- Spec: 62,263/62,263 Mac+Ubuntu (100.0%, 0 skip).
- E2E: 792/792 (Mac+Ubuntu).
- Real-world: Mac 41/50, Ubuntu 48/50 (JIT bugs W41 + wasmtime diffs W42).
- JIT: Register IR + ARM64/x86_64 + SIMD (NEON 253/256, SSE 244/256).
- HOT_THRESHOLD=3 (lowered from 10 in W38).
- Binary: 1.29MB stripped. Memory: ~3.5MB RSS.
- Platforms: macOS ARM64, Linux x86_64/ARM64, Windows x86_64.
- **main = stable**. ClojureWasm updated to v1.5.0.

## Current Task

**Phase 20: JIT Correctness Sweep** — in progress.

W38 (Lazy AOT) で HOT_THRESHOLD を 10→3 に下げた結果、以前は JIT されなかった
関数がコンパイルされるようになり、潜在 JIT バグが露出した。
Spec は 100% だが、real-world プログラムに影響がある。これらを修正するフェーズ。

### Real-world JIT failures (W41)

| Program          | Mac   | Ubuntu | 原因                                     |
|------------------|-------|--------|------------------------------------------|
| rust_compression | DIFF  | PASS   | ARM64 back-edge JIT OOB (T=10でも再現)   |
| rust_enum_match  | DIFF  | PASS   | ARM64 JIT float 化け                     |
| rust_serde_json  | DIFF  | PASS   | ARM64 JIT OOB                            |
| tinygo_hello     | DIFF  | DIFF   | ARM64+x86 共通 JIT OOB                   |
| tinygo_json      | DIFF  | DIFF   | ARM64+x86 共通 JIT OOB                   |
| tinygo_sort      | DIFF  | PASS   | ARM64 JIT 出力差異                        |

全て `--interp` で正常動作。JIT コードの correctness 問題。

### wasmtime 互換性差異 (W42, Mac のみ)

go_crypto_sha256, go_math_big, go_regex — JIT 無関係、interp でも差異。
Go runtime の env/args 処理や WASI 差異の可能性。

### Progress

**Fixed: ARM64 emitMemFill/emitMemCopy/emitMemGrow ABI register clobbering**
- `getOrLoad` returns physical registers that alias ABI arg registers (x0-x3)
- Sequential ABI arg setup clobbers vreg values before they're read
- Fix: spill all arg vregs to memory, load from regs[] into ABI regs
- x86 backend already had this fix; ARM64 did not

**Remaining: tinygo_hello correctness bugs (both platforms)**

Key finding via function exclusion bisection:
- func#154 (`os.unixFileHandle.Write`, 12 regs) produces wrong type tags
  - Output: `%!s(int=1)` instead of `arg1` — type tag corruption
  - Even with all OTHER high-reg functions excluded, func#154 alone causes wrong output
  - interfaceTypeAssert (func#124, 3 regs) returns wrong result
- Separately, func#97 + func#193 (23 regs) cause OOB via trampoline
  - The OOB chain: func#193 → func#200 (via trampoline slow path)

Verified NOT the cause:
- ABI register clobbering in BLR callsites (all verified clean)
- spillCallerSavedLive liveness analysis (tested with conservative spill, still crashes)
- Memory.fill arguments (fixed, but func#154's memory.fill args don't alias ABI regs)

### Approach (next session)

1. Focus on func#154 (12 regs, simplest crash case):
   - Add memory write trace: compare JIT vs interpreter store addresses/values
   - Check if store offsets in emitMemStore match WAT offsets
   - Verify `isConstAddrSafe` fast path correctness
2. Alternative: write a WAT module that mimics func#154's structure and test
3. Consider: is there a bug in the back-edge JIT path for func#97?

### Open Work Items

| Item     | Description                                       | Status         |
|----------|---------------------------------------------------|----------------|
| W41      | JIT real-world correctness (6 programs)           | Next session   |
| W42      | wasmtime 互換性差異 (3 Go programs, Mac)           | Low priority   |
| Phase 18 | Lazy Compilation + CLI Extensions                 | Future         |

## Completed Phases (summary)

| Phase | Name                                  | Date       |
|-------|---------------------------------------|------------|
| 1     | Guard Pages + Module Cache            | 2026-03    |
| 3     | CI Automation + Documentation         | 2026-03    |
| 5     | C API + Conditional Compilation       | 2026-03    |
| 8     | Real-World Coverage + WAT Parity      | 2026-03    |
| 10    | Quality / Stabilization               | 2026-03    |
| 11    | Allocator Injection + Embedding       | 2026-03    |
| 13    | SIMD JIT (NEON + SSE)                 | 2026-03-23 |
| 15    | Windows Port                          | 2026-03    |
| 19    | JIT Reliability                       | 2026-03    |

## References

- `@./.dev/roadmap.md` — Phase roadmap
- `@./.dev/checklist.md` — W38/W41/W42 details
- `@./.dev/references/w38-osr-research.md` — OSR research (4 approaches)
- `@./.dev/decisions.md` — architectural decisions (D131: epoch JIT timeout)
- `@./.dev/jit-debugging.md` — JIT debug techniques
- `bench/simd_comparison.yaml` — SIMD performance data
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
