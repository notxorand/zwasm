# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 + Phase 1, 3, 5, 8, 11, 15 complete. **v1.5.0** (tagged 48342ab).
- Spec: 62,263/62,263 Mac+Ubuntu+Windows (100.0%, 0 skip). E2E: 792/792. Real-world: 50/50.
- Wasm 3.0: all 9 proposals. WASI: 46/46 (100%). WAT parser complete.
- JIT: Register IR + ARM64/x86_64 (macOS, Linux, Windows x86_64). Size: 1.22MB stripped.
- **Windows x86_64**: First-class support (D129, PR #8). platform.zig abstraction,
  VEH guard pages, Win64 ABI, HostHandle WASI, Python test runners, CI 3-OS.
- **C API**: `libzwasm` (.dll/.lib, .dylib/.a, .so/.a) — 25 exported functions (D126).
- **Conditional compilation**: `-Djit=false`, `-Dcomponent=false`, `-Dwat=false` (D127).
- **main = stable**: v1.5.0. ClojureWasm updated to v1.5.0.

## Current Task

**PR #8 Windows support review + merge** (branch: `fix/pr8-review-fixes`)

- [x] Code review: 26 files, platform.zig / guard.zig / wasi.zig / x86.zig / CI
- [x] Fix run_compat.py rust_file_io regression (guest path alias)
- [x] Fix path_filestat_get SYMLINK_NOFOLLOW restoration
- [x] Fix README Stage 33 duplicate, run_spec.py .exe detection
- [x] Code quality: VEH constant, HostHandle.close(), fd placeholder docs
- [x] Doc updates: embedding.md, security.md, roadmap.md, decisions.md (D129)
- [ ] Merge Gate (Mac + Ubuntu)
- [ ] Benchmark recording
- [ ] Push to PR branch → merge via GitHub

### Pending: JIT fuel bypass + PR #6 timeout
Checklist: `@./.dev/checklist-jit-fuel-timeout.md`
PR review: `@./private/pr6-timeout-review.md`

## Handover Notes

### JIT fuel/timeout suppression — current fix vs proper solution
- **Current fix**: `jitSuppressed()` disables JIT entirely when `fuel != null`. Simple, correct, zero impact on normal execution.
- **Proper solution**: Emit fuel/deadline checks at JIT loop back-edges (like wasmtime). This preserves JIT performance even with fuel/timeout.
  - wasmtime uses negative-accumulation fuel (increment toward 0, sign check) + epoch-based timeout (atomic counter at loop headers).
  - zwasm JIT caches `vm_ptr` in x20 (ARM64) — inline `vm->fuel` decrement + conditional trampoline exit is feasible.
  - Separate future task. See `@./private/pr6-timeout-review.md` §Fix Options and wasmtime research in `~/Documents/OSS/wasmtime/crates/cranelift/src/func_environ.rs`.
- **Flaky compat tests**: W36 in checklist.md — `go_crypto_sha256`/`go_regex` intermittent DIFF on base code (pre-existing, likely W35-related).

## References

- `@./.dev/roadmap.md` (future phases), `@./.dev/roadmap-archive.md` (completed stages)
- `@./private/future/03_zwasm_clojurewasm_roadmap_ja.md` (integrated roadmap)
- `@./.dev/references/allocator-injection-plan.md` (Phase 11 design + tasks)
- `@./.dev/decisions.md`, `@./.dev/checklist.md`, `@./.dev/spec-support.md`
- `@./.dev/jit-debugging.md`, `@./.dev/bench-strategy.md`
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)

