# Reliability — Session Handover

> Plan: `@./.dev/reliability-plan.md`. Rules: `@./.claude/rules/reliability-work.md`.

## Branch
`strictly-check/reliability-003` (from main at d55a72b)

**マージ条件**: P1 (rw_c_string hang) + P2 (nbody regression) を修正するまで main にマージ禁止。
リグレッションを main に入れない。P1+P2 完了 → Merge Gate → main マージ → reliability-004 で P3-P5。

## Current: Plan A (段階的リグレッション修正 + 機能実装)

### Active Phase
**Phase 1: rw_c_string hang 修正** (Priority A)

### Phase Checklist
- [ ] **P1**: rw_c_string hang 修正 — OSR 誤爆調査
- [ ] **P2**: nbody FP キャッシュ修正 — be466a0 のリグレッション解消 (43ms→≤15ms)
- [ ] **P3**: rw_c_math 再計測 — P2 の波及効果確認、追加最適化判断
- [ ] **P4**: GC JIT 基本実装 — struct/array ops を JIT 化
- [ ] **P5**: st_matrix 許容判断 — 3.5x 以内で例外扱い

### Per-Phase Workflow (重要)
```
1. 調査: 原因特定、wasmtime 参照
2. 実装: TDD (Red→Green→Refactor)
3. 検証: zig build test + spec tests (該当時)
4. ベンチ: bash bench/run_bench.sh --quick (リグレッション確認)
5. 記録: bash bench/record.sh --id=P{N} --reason="..."  ← 必須！
6. コミット
7. 次の Phase へ
```

## Latest Benchmark Snapshot (a26a178, runs=5/warmup=3)

### >1.5x wasmtime (要対応)
| bench | zwasm | wasmtime | ratio | Phase |
|-------|------:|--------:|------:|-------|
| nbody | 43.8 | 22.0 | 1.99x | P2 |
| rw_c_math | 16.4 | 8.8 | 1.86x | P3 |
| gc_alloc | 19.2 | 10.7 | 1.79x | P4 |
| gc_tree | 138.1 | 31.4 | 4.40x | P4 |
| st_matrix | 296.3 | 91.7 | 3.23x | P5 (例外) |
| rw_c_string | ∞ | 9.3 | hang | P1 |

### ≤1.5x wasmtime (OK — 21個)
fib 0.88x, tak 0.86x, sieve 0.51x, nqueens 0.65x, tgo_tak 0.62x,
tgo_arith 0.40x, tgo_sieve 0.61x, tgo_fib_loop 0.51x, tgo_gcd 0.38x,
tgo_list 0.53x, tgo_strops 0.95x, st_sieve 0.94x, st_nestedloop 0.56x,
st_ackermann 0.48x, rw_c_matrix 0.82x, rw_cpp_string 0.49x, rw_cpp_sort 0.53x,
tgo_rwork 1.03x, tgo_fib 1.18x, tgo_nqueens 1.19x, st_fib2 1.35x, tgo_mfr 1.42x,
rw_rust_fib 1.22x

## Completed (reliability-003)
- A-F: Environment, compilation, compat, E2E, benchmarks, analysis
- G: Ubuntu spec 62,158/62,158 (100%)
- I.0-I.7: E2E 792/792 (100%), FP precision fix
- J.1-J.3: x86_64 JIT bug fixes (division, ABI, SCRATCH2, liveness)
- K.old: select/br_table/trunc_sat JIT, self-call opt, div-const, FP-direct, OSR
- Bench infra: record.sh upgraded (29 benchmarks, runs=5/warmup=3, timeout)
- history.yaml: per-commit rerun data (28 commits b39b828..ee5f585)

## Uncommitted
- `src/jit.zig`: experimental FP immediate-offset optimization (ldrFp64Imm/strFp64Imm)
  → P2 で判断。効果薄ければ revert。

## Known Bugs
- c_hello_wasi: EXIT=71 on Ubuntu (WASI issue, not JIT)
- Go WASI: 3 Go programs produce no output (WASI compatibility)
