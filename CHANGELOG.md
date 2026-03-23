# Changelog

All notable changes to zwasm are documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added
- SIMD JIT: ARM64 NEON (253/256 native) and x86_64 SSE (244/256 native) (Phase 13)
- 5 real-world SIMD benchmarks (C -msimd128): grayscale, box_blur, sum_reduce, byte_freq, nbody
- SIMD benchmark comparison infrastructure (`bench/run_simd_bench.sh`, `bench/simd_comparison.yaml`)
- JIT fuel check at back-edges: enables timeout support for JIT-compiled code (Phase 19.2)
- `--interp` CLI flag for interpreter-only execution (debugging/differential testing)
- Wide-arithmetic validation support (i64.add128, i64.sub128, i64.mul_wide_s/u)

### Fixed
- ARM64 JIT `emitGlobalSet` ABI clobber: vreg r21 overwritten before reading value (W35)
- x86_64 wide-arithmetic E2E: 19 tests fixed (validator + Debug i128 build mode)
- `i32.store16` access size in interpreter
- `--interp` flag incomplete for `doCallDirectIR` path

### Changed
- SIMD operations now JIT-compiled (was: stack interpreter only, ~22x slower)
- SIMD microbenchmarks: image_blend **4.7x**, matrix_mul **1.6x** (beats wasmtime)
- E2E runner built with ReleaseSafe (fixes x86_64 Debug i128 issues)
- E2E tests: 792/792 on both Mac and Ubuntu (was 773/792 on Ubuntu)
- Binary size: 1.23 MB stripped. Memory: ~3.5 MB RSS.

## [1.3.0] - 2026-03-02

### Added
- Module cache for predecoded IR (Phase 1.2): reuse compiled module state across invocations
- Cache-enabled benchmark variants for all bench scripts (`--cache` flag)

### Changed
- Benchmark suite expanded with cached variants for real-world programs
- CI: pinned wasmtime version to avoid broken install script on macOS
- Release process: migrated Ubuntu testing from SSH to OrbStack
- Project docs reorganized for post-v1.2.0 phase planning

## [1.2.0] - 2026-02-27

### Added
- Real-world compatibility tests: 30 programs (C, C++, Go, Rust) verified against wasmtime
- On-Stack Replacement (OSR) for back-edge JIT: enter at loop body, bypass prologue side effects
- Spec test strict mode: `--strict` flag exits non-zero on skips, enforced in CI
- 11 new unit tests (521 total)
- E2E tests expanded: 792/792 assertions (from 356)

### Fixed
- Spec validation: 87 previously skipped tests now enforced (GC typed references, subtyping, table types, exception handling catch clauses, rec group boundaries)
- Spec infra: 18 `assert_exception` tests now handled by spec runner
- E2E memory leak: `errdefer` in void-returning function was a no-op in Zig 0.15
- JIT self-call stack overflow use-after-free (E2E segfault on aarch64)
- Back-edge JIT restart corrupting Go WASI state machines (side-effect detection)
- ARM64 OSR prologue: push FP callee-saved d8-d15 (stack corruption fix)
- x86_64 OSR prologue: load physically-mapped vregs (stale register fix)
- x86_64 select aliasing: val2 clobbered when rd == val2_idx
- JIT IR instruction limit (MAX_JIT_IR_INSTRS=1500) prevents miscompilation of large functions
- Go state machine detection: br_table boundary inclusive of target instruction

### Changed
- Spec tests: 62,263/62,263 passed (100.0%, 0 skip — up from 62,158 with 105 skips)
- Benchmark results: 20/29 match or beat wasmtime, 27/29 within 1.5x (up from 14/23)
- st_sieve: 0.97x wasmtime (restored from 30.82x regression via OSR)
- GC benchmarks: gc_alloc 0.50x, gc_tree 0.73x wasmtime (JIT for struct ops)
- nbody: 0.97x wasmtime (FP cache D2-D15 expansion)
- Nightly CI: spec tests now run ReleaseSafe (eliminates Debug tail-call timeouts)

## [1.1.0] - 2026-02-18

### Added
- WAT parser spec parity: GC type annotations, memory64 syntax, typed select, NaN payloads
- WAT roundtrip 100%: 62,156/62,156 spec test modules parse and re-encode correctly
- SIMD interpreter fast-path: predecoded IR dispatch for v128 ops (~2x improvement)
- SIMD performance analysis and 3-phase optimization roadmap
- Japanese documentation (book/ja/) with language switcher
- GC benchmarks in runtime comparison recording

### Changed
- Binary size: 1.31 MB (ReleaseSafe, from 1.28 MB — WAT parser improvements)
- Runtime memory: 3.44 MB RSS (fib benchmark)
- Benchmark results: 14/23 match or beat wasmtime (up from 13/21, at v1.1.0 time)
- Updated all documentation with fresh benchmark data

## [1.0.0] - 2026-02-17

First stable release. API frozen under SemVer guarantees.

### Added
- mdBook documentation site with 12 chapters
- GitHub Pages deployment for book
- CI benchmark regression detection (20% threshold)
- CI binary size check (1.5 MB limit), ReleaseSafe build verification
- E2E tests in CI (wasmtime misc_testsuite, 356 assertions)
- Nightly sanitizer job (Debug build) and fuzz campaign (60 min)
- CI caching for Zig build artifacts
- Overnight fuzz infrastructure (`test/fuzz/fuzz_overnight.sh`)
- API boundary documentation (`docs/api-boundary.md`)
- CONTRIBUTING.md, CODE_OF_CONDUCT.md, issue templates
- Release automation: tag-triggered cross-platform builds
- Install script (`install.sh`), Homebrew formula template
- Host function and WASI examples (5 examples total)
- CHANGELOG.md
- SemVer versioning policy, deprecation guarantees

### Changed
- **BREAKING**: `loadWasi()` defaults to `cli_default` capabilities (stdio, clock, random, proc_exit). Use `loadWasiWithOptions(.{ .caps = .all })` for full access.
- `--env KEY=VALUE` injected variables now accessible without `--allow-env`
- Error messages now use human-readable format (30 error variants)
- README: badges, install section, documentation links

### Added (security)
- `--sandbox` CLI flag: deny-all capabilities + fuel 1B + memory 256MB
- `Capabilities.sandbox` preset for library API
- Restrictive library API defaults for safe embedding

## [0.3.0] - 2026-02-15

### Added
- GC proposal: 31 opcodes (struct, array, i31, cast operations)
- Threads: 79 atomic operations (load/store/RMW/cmpxchg), shared memory, wait/notify
- Exception handling: throw, throw_ref, try_table
- Function references: call_ref, br_on_null/non_null, ref.as_non_null
- Wide arithmetic: add128, sub128, mul_wide_s/u
- Custom page sizes proposal
- Multi-memory proposal
- JIT optimizations: inline self-call, smart spill, direct call, depth guard caching
- x86_64 JIT backend
- Fuel-based execution limits
- Max memory limits
- Resource limit enforcement (section counts, locals, nesting depth)
- Security audit (docs/audit-36.md, docs/security.md, SECURITY.md)
- Fuzz testing infrastructure with 25K+ corpus

### Changed
- Spec test coverage: 62,158/62,158 (100%)
- E2E tests: 356/356 from wasmtime misc_testsuite
- Binary size: 1.28 MB (ReleaseSafe)

## [0.2.0] - 2026-02-10

### Added
- Component Model: WIT parser, binary decoder, Canonical ABI, WASI P2 adapter
- WAT text format parser (`zwasm run file.wat`)
- WASI Preview 1: 46/46 syscalls (100%)
- Capability-based WASI security model
- Module linking (`--link name=file`)
- Host function imports
- Memory read/write API
- Batch mode (`--batch`)
- Inspect and validate commands
- ARM64 JIT backend
- Register IR with register allocation
- SIMD: 236 v128 opcodes + 20 relaxed SIMD

## [0.1.0] - 2026-02-08

### Added
- Initial release
- WebAssembly MVP: 172 core opcodes
- Stack-based interpreter
- Basic CLI (`zwasm run`)
- Zig library API (`WasmModule.load`, `invoke`)
