# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 + Phase 1, 3, 5 complete. **v1.3.0 released** (tagged 7570170).
- Spec: 62,263/62,263 Mac+Ubuntu (100.0%, 0 skip). E2E: 792/792 (100.0%, 0 leak).
- Wasm 3.0: all 9 proposals. WASI: 46/46 (100%). WAT parser complete.
- JIT: Register IR + ARM64/x86_64. Size: 1.20MB stripped. RSS: 4.48MB.
- Module cache: `zwasm run --cache`, `zwasm compile` (D124).
- **C API**: `libzwasm.so`/`.dylib`/`.a` — 25 exported `zwasm_*` functions (D126).
- **Conditional compilation**: `-Djit=false`, `-Dcomponent=false`, `-Dwat=false` (D127).
  Minimal build: ~940KB stripped (24% reduction).
- **main = stable**: v1.3.0 tagged. ClojureWasm updated to v1.3.0.

## Current Task

**Phase 8: Real-World Coverage + WAT Parity** — branch `phase8/real-world-wat`.

### Phase 8.1 Complete: 50 real-world programs (was 30)
- C1: TinyGo infra + 4 programs (hello, fib, sort, json) — `-scheduler=none`
- C2: SHA-256 + miniz C programs
- C5: C data structures (regex, utf8, btree, lz4)
- C6: Rust crate programs (regex, serde_json, sha256, compression)
- C7: Go + C++ (crypto_sha256, regex, json_parse)
- C8: Stress tests (deep_recursion, large_memory, many_functions)
- All 50 PASS compat test (zwasm vs wasmtime)
- Deferred: SQLite + Lua (JIT bug W30)

### Phase 8.2 Complete: WAT parity 100%
- WAT roundtrip: 62,259/62,259 passed (100.0%)
- 708 conv-fail = wasm-tools can't convert malformed .wasm (expected)
- No WAT parser fixes needed

### W30 JIT Bug Fix (Phase 8 pre-merge)
- Guard page recovery: save/restore across nested JIT calls (SIGBUS fix)
- instrDefinesRd: global.set/memory.fill/memory.copy rd is USE not DEF
- computeCalleeSavedLiveSet: added rd-as-USE + select condition vreg
- x86 emitCall: removed liveness-aware spill/reload (caused register file
  corruption — non-live phys regs had garbage after CALL, subsequent
  spillCallerSaved wrote garbage to register file)
- ARM64 spillCallerSavedLive: reverted "spill ALL" back to "spill live only"
  (the "spill ALL" caused intermittent failures in Go programs)
- emitInlineSelfCall: moved emitLoadMemCache before reloadCallerSavedLive
  (BLR clobbers caller-saved regs; also fixed Ubuntu TinyGo 0.37.0 OOB crash)
- Mac: 50/50 PASS (W31 resolved — bad test data, not JIT bug)
- Ubuntu: 50/50 PASS, 0 CRASH. Spec 62,263/62,263. E2E 792/792.

### Merge Gate PASSED
- Mac: unit PASS, spec 62,263/62,263, E2E 792/792, compat 50/50, binary 1.20MB
- Ubuntu: unit PASS, spec 62,263/62,263, E2E 792/792, compat 50/50

**Next**: Merge to main + update compat count.

## References

- `@./.dev/roadmap.md` (future phases), `@./.dev/roadmap-archive.md` (completed stages)
- `@./private/future/03_zwasm_clojurewasm_roadmap_ja.md` (integrated roadmap)
- `@./.dev/decisions.md`, `@./.dev/checklist.md`, `@./.dev/spec-support.md`
- `@./.dev/jit-debugging.md`, `@./.dev/bench-strategy.md`
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
