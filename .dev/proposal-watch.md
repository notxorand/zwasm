# Proposal Watch

Active WebAssembly proposals tracked for zwasm compatibility.
Updated when proposals advance phases or new proposals become relevant.

## Phase 4 (Standardize the Feature)

| Proposal | Phase | Impact | Notes |
|----------|-------|--------|-------|
| Stack Switching | 4 | High | Continuations, async. Deferred until spec stabilizes. |
| Memory Control | 4 | Medium | memory.discard, memory.fill optimizations. |
| JS String Builtins | 4 | Low | JS host-specific, not relevant for standalone runtime. |

## Phase 3 (Implementation Phase)

| Proposal | Phase | Impact | Notes |
|----------|-------|--------|-------|
| Branch Hinting | 3 | Low | Custom section hints for JIT. Low priority. |
| Shared-Everything Threads | 3 | High | Shared memory + atomics across modules. |
| Wide Arithmetic | 3 | Medium | i64x2 multiply, add-with-carry. |

## Phase 2 (Proposed Spec Text)

| Proposal | Phase | Impact | Notes |
|----------|-------|--------|-------|
| Profiles | 2 | Medium | Deterministic profile (no NaN payload). |
| Flexible Vectors | 2 | Low | Variable-length SIMD. Deferred. |

## Monitoring Strategy

- **SpecTec monitor** (`.github/workflows/spectec-monitor.yml`): Weekly check on spec repo changes.
- **Spec bump** (`.github/workflows/spec-bump.yml`): Weekly auto-PR for test suite updates.
- **wasm-tools bump** (`.github/workflows/wasm-tools-bump.yml`): Monthly tool version check.
- **Manual review**: Check proposal repos quarterly for phase advances.
