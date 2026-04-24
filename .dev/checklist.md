# Deferred Work Checklist

Open items only. Resolved items in git history.
Prefix: W## (to distinguish from CW's F## items).

## Invariants (always enforce)

- [ ] D100: No CW-specific types in public API (Value, GC, Env)
- [ ] D102: All types take allocator parameter, no global allocator

## Open Items

- [ ] W46: Un-link libc — migrate WASI I/O / cache / platform off `std.c.*` onto
  `std.Io` (or `std.posix.system` direct syscalls). Scope: `src/wasi.zig`
  (fd_read / fd_write / fd_pread / fd_pwrite / fd_seek / fd_tell / fstatat),
  `src/cache.zig` (fsync), `src/platform.zig` (statx helpers), test helpers.
  Why this is a priority, not cosmetic:
  1. **Zig 0.16 philosophy** — release notes explicitly say "go higher
     (std.Io) or go lower (std.posix.system)"; `std.c.*` is neither direction.
  2. **Windows correctness** — `std.c.fd_t` = `windows.HANDLE` (pointer),
     but `std.c.write` binds to MSVCRT `_write(int fd, …)`. The ABI
     mismatch is what broke Windows real-world compat in v1.10.0.
  3. **Binary size** — Linux ELF pays ~290 KB for libc linkage; returning
     to no-libc lets us restore the 1.50 MB guard.
  4. **Performance** — 2025-02 Andrew Kelley devlog: "No-Libc Zig Now
     Outperforms Glibc Zig" — no-libc is the fast path.
  Non-goal for v1.10.x: rewriting all I/O atomically. Plan is phase-by-phase
  (phase 1 = WASI stdio Windows fix, phase 2 = fd_read/fd_write, …).

- [ ] W45: SIMD loop persistence — Skip Q-cache eviction at loop headers.
  Requires back-edge detection in scanBranchTargets.

- [ ] W47: `tgo_strops_cached` +24% regression post-0.16 (v1.9.1 64.5ms →
  v1.10.0 79.9ms on Mac aarch64). Only single benchmark out of 46+ that
  regressed >10% AND >=10ms absolute. Investigate TinyGo strops codegen
  path — likely regalloc or memory-access pattern change. Low priority
  since 20 other benchmarks improved >10% (GC paths 40–76% faster).

## Resolved (summary)

W37: Contiguous v128 storage. W39: Multi-value return JIT (guard removed).
W40: Epoch-based JIT timeout (D131).
W38: SIMD JIT C-compiled perf — Lazy AOT (HOT_THRESHOLD 10→3, back_edge_bailed,
     extract_lane fix, memory_grow64 fix, cross-module instance fix).
W41: JIT real-world correctness — ALL FIXED (Mac 49/50, Ubuntu 50/50).
     void-call reloadVreg, written_vregs pre-scan, void self-call result,
     ARM64 fuel check x0 clobber (tinygo_sort), stale scratch cache in signed
     div (rust_enum_match). Fixed through 2026-03-25.
W42: go_math_big — FIXED (remainder rd==rs1 aliasing in emitRem32/emitRem64).
     emitRem used UDIV+MSUB; UDIV clobbered dividend before MSUB could use
     it. Fix: save rs1 to SCRATCH before division when d aliases rs1.
     Fixed 2026-03-25.
W43: SIMD v128 base addr cache (SIMD_BASE_REG x17). Phase A of D132.
W44: SIMD register class — Q16-Q31 (ARM64) + XMM6-XMM15 (x86) cache.
     Phase B of D132. Merged 2026-03-26. Q-cache with LRU eviction + lazy
     writeback. Benefit limited by loop-header eviction (diagnosed same day).

W2-W36: See git history. All resolved through Stages 0-47 and Phases 1-19.
