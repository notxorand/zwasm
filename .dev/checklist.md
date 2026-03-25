# Deferred Work Checklist

Open items only. Resolved items in git history.
Prefix: W## (to distinguish from CW's F## items).

## Invariants (always enforce)

- [ ] D100: No CW-specific types in public API (Value, GC, Env)
- [ ] D102: All types take allocator parameter, no global allocator

## Open Items

- [x] W41: JIT real-world correctness — ALL FIXED (Mac 49/50, Ubuntu 50/50)
  Phase 20 fixed: void-call reloadVreg, written_vregs pre-scan, void self-call result,
  ARM64 fuel check x0 clobber (tinygo_sort), **stale scratch cache in signed div**
  (rust_enum_match fixed 2026-03-25).

- [x] W42: go_math_big — FIXED (remainder rd==rs1 aliasing in emitRem32/emitRem64)
  Root cause: NOT env-dependent — was a JIT bug. emitRem used UDIV+MSUB but
  when rd==rs1, UDIV clobbered the dividend before MSUB could use it.
  Fix: save rs1 to SCRATCH before division when d aliases rs1.
  go_math_big now PASS on Mac. Fixed 2026-03-25.

## Resolved (summary)

W37: Contiguous v128 storage. W39: Multi-value return JIT (guard removed).
W40: Epoch-based JIT timeout (D131).
W38: SIMD JIT C-compiled perf — Lazy AOT (HOT_THRESHOLD 10→3, back_edge_bailed,
     extract_lane fix, memory_grow64 fix, cross-module instance fix).
W41 (partial): void-call reloadVreg fix — emitCall/emitCallIndirect skips
     reloadVreg(rd) when n_results=0. Fixes rust_compression, rust_serde_json,
     rust_enum_match (+3 Mac, stable Ubuntu). n_results encoded in rs2_field.

W43: SIMD v128 base addr cache (SIMD_BASE_REG x17). Phase A of D132.
W44: SIMD register class — Q16-Q31 (ARM64) + XMM6-XMM15 (x86) cache.
     Phase B of D132. Merged 2026-03-26. Q-cache with LRU eviction + lazy
     writeback. Benefit limited by loop-header eviction (diagnosed same day).
W45: SIMD loop persistence — NEXT. Skip Q-cache eviction at loop headers.
     Requires back-edge detection in scanBranchTargets.

W2-W36: See git history. All resolved through Stages 0-47 and Phases 1-19.
