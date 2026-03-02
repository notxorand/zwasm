# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 complete. v1.2.0 released. ~50K LOC, 521 unit tests.
- Spec: 62,263/62,263 Mac+Ubuntu (100.0%, 0 skip). E2E: 792/792 (100.0%, 0 leak).
- Wasm 3.0: all 9 proposals. WASI: 46/46 (100%). WAT parser complete.
- JIT: Register IR + ARM64/x86_64. Size: 1.19MB / 1.52MB RSS.
- **main = stable**: ClojureWasm depends on main (v1.2.0 tag).

## Current Task

Phase 1 complete. Ready for Merge Gate → v1.3.0.

### Completed: Phase 1 — Guard Pages + Module Cache

**1.1 Guard Pages**: Already implemented (guard.zig, memory.zig, store.zig, jit.zig,
x86.zig, cli.zig). JIT bounds check elimination active.

**1.2 Module Cache (D124)**: Implemented. `cache.zig` serializes predecoded IR
(`PreInstr` + `pool64`) to `~/.cache/zwasm/<hash>.zwcache`.
CLI: `zwasm run --cache` (auto-load/save), `zwasm compile` (AOT predecode).
Mac Commit Gate passed: spec 62,263, E2E 792/792, real-world 30/30.

**Next**: Merge Gate (Mac + Ubuntu), then v1.3.0 release.
After that: Phase 3 (CI Automation + Documentation).

Previous: v1.2.0 released (tagged 5d54ae9, CW updated).

## Known Bugs

None.

## References

- `@./.dev/roadmap.md` (future phases), `@./.dev/roadmap-archive.md` (completed stages)
- `@./private/future/03_zwasm_clojurewasm_roadmap_ja.md` (integrated roadmap)
- `@./.dev/decisions.md`, `@./.dev/checklist.md`, `@./.dev/spec-support.md`
- `@./.dev/jit-debugging.md`, `@./.dev/bench-strategy.md`
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
