# zwasm Roadmap

Independent Zig-native WebAssembly runtime — library and CLI tool.
Benchmark target: **wasmtime**. Optimization target: ARM64 Mac first.

**Library Consumer Guarantee**: ClojureWasm depends on zwasm main via GitHub URL.
All development on feature branches; merge to main requires Merge Gate.

## Completed (v1.2.0)

Stages 0-46 complete. Details: `roadmap-archive.md`.

- Wasm 3.0: all 9 proposals (581+ opcodes). WASI P1 46/46. WAT parser.
- JIT: Register IR + ARM64/x86_64. Spec: 62,263/62,263 (100%, 0 skip).
- E2E: 792/792 (0 leak). Real-world: 30/30. Fuzz: 10K+ iterations, 0 crashes.
- Size: 1.19MB stripped / 1.52MB RSS. Mac + Ubuntu x86_64.

## Future Phases (zwasm only)

Integrated roadmap (zwasm + CW): `private/future/03_zwasm_clojurewasm_roadmap_ja.md`
CW-specific phases (2, 4, 6, 7, 9, 14, 16, 17) are in that document only.

### Phase 1: Guard Pages + Module Cache — COMPLETE

Performance impact: highest of remaining items. Improvements propagate to CW.

**1.1 Virtual Memory Guard Pages — COMPLETE**

Already implemented: guard.zig (mmap/mprotect/signal handler), memory.zig (initGuarded),
store.zig (auto-use when JIT supported), jit.zig/x86.zig (bounds check elimination),
cli.zig (signal handler install).

**1.2 Module Cache / AOT Serialize — COMPLETE (D124)**

- `cache.zig`: serialize predecoded IR to `~/.cache/zwasm/<hash>.zwcache`
- `zwasm run --cache` option + `zwasm compile` command
- Cache invalidation: SHA-256 hash + version field
- All tests pass (spec 62,263/62,263, E2E 792/792, real-world 30/30)

**Gate**: PASSED. zwasm v1.3.0 candidate.

### Phase 3: CI Automation + Documentation (2 days)

**3.1 CI Automation (1 day)**

- Spec submodule auto-bump (weekly cron)
- wasm-tools version auto-update (monthly cron)
- SpecTec monitoring
- `.dev/proposal-watch.md` (Phase 3-4 proposal watchlist)

**3.2 Documentation (1 day)**

- `ARCHITECTURE.md` (4-layer execution pipeline diagram + file mapping)
- `///` doc comments on all source files
- `docs/data-structures.md` (glossary)
- decisions.md: add affected file references

**Gate**: CI cron working. ARCHITECTURE.md in place.

### Phase 5: C API + Conditional Compilation (3 days)

**5.1 C API (wasm-c-api) (2 days)**

- D## decision record (D125)
- `c_api.zig`: export engine/store/module/instance/func/memory/val via C ABI
- WASI config C API
- `include/zwasm.h` header generation
- `libzwasm.so` / `libzwasm.dylib` shared library build
- C test + Python ctypes example

**5.2 Conditional Compilation (1 day)**

- `-Djit=false`, `-Dsimd=false`, `-Dgc=false`, `-Dthreads=false`, `-Dcomponent=false`
- Minimal build (MVP+WASI, no JIT) target < 500KB
- CI size matrix

**Gate**: C API tests pass. Minimal build < 500KB.

### Phase 8: Real-World Coverage + WAT Parity (3 days)

**8.1 Real-World Coverage Expansion (2 days)**

- High-priority targets: SQLite, Lua WASI, PHP WASI
- Toolchain tests: Emscripten, AssemblyScript, TinyGo
- WasmBench evaluation (WASI-compatible module batch execution)
- Stress tests (large functions, deep recursion, large memory)
- Compatibility metric (target 95%+, 50+ programs)

**8.2 WAT Spec Parity (1 day)**

- WAT roundtrip audit script
- Gap triage → categorical fixes
- Input validation hardening
- GC type annotation support

**Gate**: real-world 50+ / WAT roundtrip rate 100% (where feasible).

### Phase 10: Quality / Stabilization (zwasm portion, 1 day)

- Full test suite re-verification (Mac + Ubuntu)
- Benchmark regression check
- Size guard confirmation (≤ 1.5MB)
- Merge Gate pass

**Gate**: All test suites pass. No regressions.

### Phase 13: SIMD JIT (5 days)

Largest technical challenge.

- SIMD microbenchmark suite
- RegIR v128 register class extension
- ARM64 NEON + x86 SSE codegen (top 20 instructions)
- Target: st_matrix gap 22.3x → ≤ 5x
- Size guard ≤ 1.5MB

**Gate**: SIMD bench faster than scalar. zwasm v2.0.0 candidate.

### Phase 15: Windows Port (3 days)

- D## decision record (SEH, VirtualAlloc, CI strategy)
- Memory management OS abstraction (mmap → VirtualAlloc)
- Signal handler port (SIGSEGV → SEH)
- JIT W^X port (VirtualProtect + FlushInstructionCache)
- WASI filesystem Windows branch
- CI Windows job + release binaries

**Gate**: Windows x86_64 all tests pass. 3-OS support complete.

### Phase 18: Book i18n + Lazy Compilation + CLI Extensions (3 days)

**18.1 Book i18n (1 day)**

- `book/en/` + `book/ja/` structure
- Japanese translation + language switcher

**18.2 Lazy Compilation (1 day)**

JIT deferred to first call. Trampoline → direct jump patch.

**18.3 CLI Extensions (1 day)**

- `zwasm dump` (detailed module dump)
- `zwasm bench` (built-in benchmark runner)
- `zwasm diff` (module comparison)
- Fuel API docs + Memory growth callback

## Future (timeline TBD)

| Item | Condition |
|------|-----------|
| Stack Switching | Proposal reaches Phase 4 |
| WASI P3 / Async | After wasmtime stabilizes |
| Copy-and-Patch JIT | When technique matures |
| GC collector upgrade (generational/Immix) | D121 arena approach sufficient for now |
| Liveness-based regalloc / LIRA | Rejected (D116/D120), single-pass sufficient |

## Version Milestones

| Version | Phases | Key Results |
|---------|--------|-------------|
| **v1.3.0** | 1, 3 | Guard pages, cache, CI automation, ARCHITECTURE.md |
| **v1.4.0** | 5, 8 | C API, conditional compilation, 50+ real-world, WAT parity |
| **v2.0.0** | 13, 15 | SIMD JIT, Windows, 3-OS support |

## Benchmark History

| Milestone          | fib(35) | vs wasmtime |
|--------------------|---------|-------------|
| Stage 0 (baseline) | 544ms   | 9.4x        |
| Stage 3 (3.14)     | 103ms   | 2.0x        |
| Stage 5 (5.7)      | 97ms    | 1.72x       |
| Stage 23           | 91ms    | 1.8x        |
| Stage 25           | 52ms    | 1.0x        |
