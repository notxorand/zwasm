# zwasm Development Memo

Session handover document. Read at session start.

## Current State

- Stages 0-46 + Phase 1 complete. **v1.3.0 released** (tagged 7570170).
- Spec: 62,263/62,263 Mac+Ubuntu (100.0%, 0 skip). E2E: 792/792 (100.0%, 0 leak).
- Wasm 3.0: all 9 proposals. WASI: 46/46 (100%). WAT parser complete.
- JIT: Register IR + ARM64/x86_64. Size: 1.20MB stripped. RSS: 4.48MB.
- Module cache: `zwasm run --cache`, `zwasm compile` (D124).
- **main = stable**: v1.3.0 tagged. ClojureWasm updated to v1.3.0.

## Current Task

Phase 3 (CI Automation + Documentation) complete on `phase3/ci-docs`.

**Done**:
- CI: centralized tool-versions, spec-bump, wasm-tools-bump, spectec-monitor workflows
- Nightly re-enabled as weekly
- D125 decision, proposal-watch.md
- ARCHITECTURE.md, docs/data-structures.md
- Doc comments on fuzz files, affected-file refs on all D## entries

**Next steps**:
1. Merge `phase3/ci-docs` to main (after Merge Gate)
2. Phase 5: C API + Conditional Compilation (see `roadmap.md`)

## Known Bugs

None.

## References

- `@./.dev/roadmap.md` (future phases), `@./.dev/roadmap-archive.md` (completed stages)
- `@./private/future/03_zwasm_clojurewasm_roadmap_ja.md` (integrated roadmap)
- `@./.dev/decisions.md`, `@./.dev/checklist.md`, `@./.dev/spec-support.md`
- `@./.dev/jit-debugging.md`, `@./.dev/bench-strategy.md`
- External: wasmtime (`~/Documents/OSS/wasmtime/`), zware (`~/Documents/OSS/zware/`)
