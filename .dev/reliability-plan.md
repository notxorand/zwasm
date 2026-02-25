# zwasm Codebase Reliability Improvement — Execution Plan

> Created: 2026-02-25
> Branch: `strictly-check/reliability`
> Origin: `private/20260225_strictly_check/01_plan.md` (user requirements)
> Handover: `.dev/reliability-handover.md`

## Motivation

Social media feedback raised concerns about:
1. "Doesn't work in my environment"
2. "Benchmarks seem biased toward zwasm"
3. "Is Wasm 3.0 truly supported?"
4. "Is WASI 2.0 truly supported?"

Goal: Make zwasm **undeniably correct and fast** — anyone on Mac (aarch64) or Ubuntu (x86_64)
can verify every claimed feature works, and benchmarks are fair and reproducible.

## Current Baseline

- Spec: 62,158/62,158 (100%) Mac + Ubuntu
- E2E: 341/356 (95.8%) — 15 failures (call_indirect GC subtypes, table_copy imported)
- Benchmarks: 23 benchmarks (WAT 5, TinyGo 11, Shootout 5, GC 2)
  - 14/23 match or beat wasmtime. Weak spots: st_fib2 (1.54x), st_matrix (3.3x), gc_tree (4.3x)
- Known bugs: JIT nested loop 9+ vregs (W34)
- Toolchains available: Rust (wasm32-wasip1), Go (wasip1/wasm), TinyGo, C/C++ (wasi-sdk)
- flake.nix: Zig + wasmtime + bun + node + tinygo + Go + wasi-sdk

## Phase Overview

| # | Phase | Description | Depends |
|---|-------|-------------|---------|
| A | Environment Setup | flake.nix + toolchains, branch creation | — |
| B | Real-World Wasm Compilation | Build wasm from Rust, Go, C, C++ | A |
| C | Compatibility Testing | Run all compiled wasm on zwasm vs wasmtime | B |
| D | E2E Test Expansion | Full E2E for all claimed features | C |
| E | Benchmark Expansion | Real-world benchmarks, fair comparison | B |
| F | Performance Investigation | Fix cases where zwasm is significantly slower | E |
| G | Ubuntu Cross-Platform | Full verification on Ubuntu x86_64 | A-F |
| H | Documentation Accuracy | Ensure README/docs match reality | D,E |

---

## Phase A: Environment Setup

### A.1: Create feature branch
```bash
git checkout -b strictly-check/reliability main
```

### A.2: Expand flake.nix
Add: Go (wasip1/wasm), wasi-sdk 30 (C/C++ → wasm32-wasi).
Rust uses system rustup (wasm32-wasip1 target pre-installed).

### A.3: Verify flake.nix on Ubuntu
```bash
ssh ubuntu 'bash -l -c "cd ~/zwasm && git pull && nix develop --command bash -c \"go version && zig version\""'
```

---

## Phase B: Real-World Wasm Compilation

Create `test/realworld/` directory with subdirectories per language.

### B.1: Rust programs → wasm32-wasip1
- `hello_wasi` — stdout, args, env vars
- `fib_compute` — pure computation
- `file_io` — read/write files via WASI
- `json_parse` — serde_json (real dependency, allocator stress)

Build: `cargo build --target wasm32-wasip1 --release`

### B.2: Go programs → wasip1/wasm
- `hello_wasi` — fmt.Println, os.Args
- `sort_benchmark` — sort large slices (GC + computation)
- `json_marshal` — encoding/json

Build: `GOOS=wasip1 GOARCH=wasm go build -o program.wasm`

### B.3: C programs → wasm32-wasi (via wasi-sdk)
- `hello_wasi` — printf, argc/argv
- `matrix_multiply` — dense computation
- `string_processing` — string manipulation
- `math_compute` — math.h functions

Build: `$WASI_SDK_PATH/bin/clang --sysroot=$WASI_SDK_PATH/share/wasi-sysroot -O2 -o prog.wasm prog.c`

### B.4: C++ programs → wasm32-wasi
- `vector_sort` — std::vector + std::sort
- `string_ops` — std::string operations

Build: same as C but with clang++ and `-fno-exceptions`

### B.5: Build automation
`test/realworld/build_all.sh` — builds all, reports pass/fail.

---

## Phase C: Compatibility Testing

### C.1: Compatibility test runner
`test/realworld/run_compat.sh`:
- Run each wasm with wasmtime and zwasm
- Compare stdout, stderr, exit code
- Report PASS / DIFF / CRASH / UNSUPPORTED

### C.2: Fix any failures
Root-cause analysis → fix → re-test.

### C.3: Document unsupported cases
`test/realworld/KNOWN_LIMITATIONS.md` for legitimate unsupported features.

---

## Phase D: E2E Test Expansion

### D.1: Fix existing E2E failures
15 failures: call_indirect (13, GC subtype), table_copy_on_imported_tables (2).

### D.2: Feature-specific E2E tests
Comprehensive E2E for: SIMD, EH, GC, Threads, Memory64, Tail calls,
Multi-memory, Function references, WAT parser, WASI P1/P2.

### D.3: Update E2E runner

---

## Phase E: Benchmark Expansion

### E.1: Real-world benchmarks
Add Rust, Go, C benchmarks (computation, allocator stress, string heavy).

### E.2: Benchmark harness update
`--layer=realworld` support in `bench/compare_runtimes.sh`.

### E.3: Fair benchmark audit
Ensure JIT warmup is fair, hyperfine `--warmup 3 --min-runs 5`.

### E.4: Record baseline

---

## Phase F: Performance Investigation

### F.1: Analyze weak spots
st_fib2 (1.54x), st_matrix (3.3x), gc_tree (4.3x), gc_alloc (2.2x).

### F.2: Profile and optimize
For >2x gaps: profile → study cranelift → implement → measure.

### F.3: JIT nested loop fix (W34)

---

## Phase G: Ubuntu Cross-Platform Verification

Full test suite, real-world compat, benchmarks on Ubuntu x86_64.
Fix any platform-specific failures.

---

## Phase H: Documentation Accuracy

### H.1: Audit README claims
Verify every claim (opcode count, spec %, proposals, benchmarks).

### H.2: Fix discrepancies
Implementation first, docs update if truly infeasible.

### H.3: Update benchmark table

---

## Execution Order

```
A (env) → B (compile) → C (compat) + D (e2e) → E (bench) → F (perf) → G (ubuntu) → H (docs)
```

## References

- `.dev/memo.md` — current state
- `.dev/reliability-handover.md` — progress tracker
- `.dev/checklist.md` — deferred items
- `.dev/spec-support.md` — opcode/WASI coverage
- `bench/runtime_comparison.yaml` — benchmark data
- `private/roadmap-production.md` — production roadmap
