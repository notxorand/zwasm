# zwasm v2 charter — motivation, context, discussion seeds

Captured 2026-04-30 at the end of the W54 redesign session. Written
**before** any v2 work begins. Treat this as a starting brief for
the v2 investigation, not a plan to execute.

> **Status**: Pre-investigation. v2 has not started. The next
> session's first action is to read this document, then enter a
> deliberate research-and-redesign phase. No v2 code commits until
> a charter-derived ROADMAP exists.

## 1. Why this document exists

zwasm v1 (current `main`) works. It ships spec / e2e / realworld
green on three OSes and is the runtime ClojureWasm depends on. The
question this document opens is **whether to start a parallel v2
ground-up rewrite** alongside v1, on the model of the
`cw-from-scratch` branch in `~/Documents/MyProducts/ClojureWasmFromScratch/`.

The motivation is **not "the v1 codebase is broken and must be
replaced"**. v1 is a perfectly serviceable runtime. The motivation
is structural and long-term — it's about what kind of codebase
the user wants to maintain over the next several years, what
investments compound, and what knowledge gets compressed into the
artefact vs scattered across patches.

This document captures the framing so the next session — and
sessions after that — can pick up the thread without having to
reconstruct the reasoning each time.

## 2. The four motivations (as articulated by the user, 2026-04-30)

These are the load-bearing reasons. v2's value rests on whether each
of these is genuinely true, and whether v2 is the **only** way to
realise them.

### 2.1 "Structurally clean codebase"

Status of v1: `src/jit.zig` 8769 lines, `src/x86.zig` 7620 lines,
`src/vm.zig` 10546 lines. The W54 redesign session showed concretely
that the complexity isn't from too many features — it's from
implicit-contract sprawl across two backends + regalloc + JIT-side
caching state, where each layer has accumulated assumptions about
the others without ever being formally written down.

Phase 5's coalescer broke `go_math_big` on x86_64 specifically
because regalloc-stage IR contracts and x86 backend's local
caching assumptions had drifted. Mac aarch64 didn't trigger it.
This is the kind of bug that **mechanical refactoring of v1 cannot
prevent**, because the contract has to exist on day 1 to be useful.

The v1 path forward: continue patching, add `regalloc.verify()`
post-hoc, slowly carve out a VCode-like layer. Each step is
non-trivial, each is a separate PR, and the cumulative result
trends toward the v2 design anyway — but lacks the consolidation
that comes from designing the layers together.

### 2.2 "Lower modification cost as Wasm evolves"

Wasm is not finished. WASI 0.3 / Component Model maturity / WasmGC
finalisation / branch-hinting / multimemory / FP-relaxed-rounding
/ exception-handling refinement / atomic memory operations —
each one touches VM state, IR shape, and JIT codegen.

In v1, adding an opcode currently means: edit `opcode.zig` (case),
edit `vm.zig` (interpreter case), edit `predecode.zig`
(classification), edit `regalloc.zig` (RegInstr mapping), edit
`jit.zig` (ARM64 emit) and `src/x86.zig` (x86 emit), and possibly
`simd_arm64.zig` for SIMD. Six files, **each backend's invariants
implicit**, each parallel implementation a maintenance vector.

In a v2 with a VCode-like middle layer (cranelift's pattern, or
SpiderMonkey's), the same opcode addition is one mid-layer op +
two emit one-liners. That ratio compounds over years of Wasm
evolution. The crossover point — where "v1 maintenance cost over
its remaining life" exceeds "v2 build cost" — depends on Wasm's
churn rate, which has historically been steady.

### 2.3 "FFI standardisation toward industry norms"

zwasm v1's `c_api.zig` exists but is zwasm-specific. The
industry-standard C ABI for embedding Wasm runtimes is
[wasm-c-api](https://github.com/WebAssembly/wasm-c-api), used by
wasmtime, wasmer, V8, and SpiderMonkey. Programs written against
that header (Emacs's wasm-mode dependencies, postgres-wasm
extensions, edge runtime hosts) port between runtimes by relinking.

v1 cannot retro-fit wasm-c-api compliance without ABI break:
existing zwasm-specific call sites (wherever they exist) would
need to migrate. v2 can implement wasm-c-api **first**, then add
zwasm-specific extensions as additional symbols. Host applications
written against the standard then drop in zwasm in place of
wasmtime / wasmer with no source change.

This is a value-multiplier specifically because zwasm has few
users today. The cost of breaking v1's API on existing call sites
is approximately zero now; in two years it may not be.

### 2.4 "Knowledge compression"

v1 is the user's first attempt at a Wasm runtime. The 1.5-year
build-out generated knowledge that is currently scattered:

- `.dev/decisions.md` D100-D137 are individually clear but the
  **overall design synthesis** ("what should the layer
  boundaries have been") is not captured.
- Inflection-point insights — `inst_ptr_cached` register
  contention, `written_vregs` full-function pre-scan, the
  `simd_base_cached` x17 reservation, the `prologue_load_mask`
  computation — are individually optimal but the meta-pattern
  "JIT needs an explicit register-class abstraction" is implied,
  not stated.
- Single-pass JIT vs two-arch maintenance trade-offs are baked
  into the architecture but never argued through; questioning
  them now requires reading both backends end-to-end.

A ground-up rewrite is, mechanically, **the act of forcing the
synthesis** — every layer boundary gets re-decided, every
unstated invariant gets surfaced, and the result is a codebase
that reads like a textbook rather than a deposition.

ClojureWasm v2's `.dev/ROADMAP.md` (~7000 lines) + ADR series +
the `docs/ja/learn_clojurewasm/` chapter series demonstrate this
pattern in practice: the documentation is **not retrospective
explanation**, it's the design artefact itself.

For zwasm v2, the equivalent would be `docs/ja/learn_zwasm/`
chapters paired with each phase. The user has stated this style of
documentation is part of the goal, not a side benefit.

## 3. Why ClojureWasm v2 is a useful precedent

`~/Documents/MyProducts/ClojureWasmFromScratch/` is a parallel-
universe rewrite of ClojureWasm started 2026-04-21 (10 days ago
as of this charter). Read its `CLAUDE.md` and `.dev/ROADMAP.md`
in full before starting v2 work — those documents are the
template, not just a reference.

Key patterns from CW v2 that translate directly:

| CW v2 pattern | What it gives zwasm v2 |
|---|---|
| **P2: See the final shape on day 1** | Directory layout fixed before any code — `src/{front,mid,back}` or equivalent decided up front |
| **A1: zone_check.sh CI gate** | Layer dependency direction enforced mechanically; "lower zones don't import upper" |
| **A6: ≤ 1000 lines per file (soft)** | jit.zig 8769 lines → physically prevented |
| **P12: Dual backend with `--compare`** | zwasm has TWO backends (ARM64, x86_64). Differential testing was the missing safety net for Phase 5's `go_math_big` regression |
| **🔒 x86_64 Gate per phase** | OrbStack Ubuntu mandatory before next phase begins |
| **P4: No ad-hoc patches; ADR or escalate** | Register-collision-class problems become ADR triggers, not silent merges |
| **ADR template + numbering** | Decisions become traceable artefacts with rejected-alternatives sections |
| **`docs/ja/learn_*` chapters per phase** | Knowledge compression by construction |
| **`continue` / `audit_scaffolding` skills** | Phase-boundary discipline automated |
| **`private/` for scratch + `.dev/` for canonical** | Exploration vs commitment separated mechanically |
| **`scripts/zone_check.sh --gate`** | Architectural invariants verifiable in CI |

CW v2 has reached Phase 4 in 10 days with this discipline.
zwasm v2 starts from a similar position (existing v1 as
reference) and can reuse the entire procedural toolkit.

## 4. Discussion seeds (話のタネ — neither mandates nor commitments)

These are improvements that **might** be in scope. Each is a
candidate for early-phase deliberation, not a guaranteed v2
feature. The investigation phase exists precisely to argue
through which of these matter.

### 4.1 Architecture seeds

- **VCode-like arch-agnostic middle layer**. Both ARM64 and x86
  backends consume the same VCode stream; arch-specific code is
  only the final `emit*` step. (cranelift's structure;
  SpiderMonkey calls it MIR → LIR.)
- **Explicit RegFunc well-formedness contract**. `regalloc.verify(rf)
  -> WellFormed | error{...}` runs after every regalloc-stage pass,
  in CI. Phase 5's bug class becomes detectable as
  contract-violation, not as backend mismatch.
- **`RegPool` / register-class abstraction from day 1**. The
  W54 hoist's `inst_ptr_cached` collision is a symptom of having
  no register-class concept — v2 starts with one even if only
  one class is populated initially.
- **Single-pass JIT — keep, drop, or hybridise?** v1's choice is
  defensible (cold-start cost) but never argued in writing.
  Alternatives: two-pass JIT for HOT functions only, e-graph
  midend for tier-2, copy-and-patch (the StackVM trend).
- **Liveness as a first-class IR property**. v1 added it post-hoc
  in W54-Phase-1 and dropped it for the x86 regression. v2 can
  bake it into RegFunc construction.
- **Bounds-check elision strategy from the start**. v1 elides
  only constant-known addresses; cranelift uses guard pages +
  static analysis. The choice has implications for the JIT
  contract.

### 4.2 API / FFI seeds

- **wasm-c-api compliance as the C ABI**. Discussed in §2.3.
- **Re-think the public Zig API**. v1's `types.zig` exposes a
  pragmatic surface; v2 can choose between "thin" (Zig idiom,
  small) and "rich" (parity with the C API, larger).
- **Component Model first-class support**. v1 has scaffolding
  (`canon_abi.zig`, `component.zig`) but Component is gated
  behind `enable_component`. v2 can decide whether Component
  is core or a module.
- **Embedder hook design**. Host function injection, custom
  allocators (D128 in v1's decisions.md), stdio override,
  cancellation, fuel — these were added incrementally to v1.
  v2 can design the hook surface coherently.

### 4.3 Test infrastructure seeds

- **Differential testing across {interpreter, ARM64 JIT, x86 JIT}
  from day 1**. The same wasm runs in all three, stdout/stderr
  compared. This is the safety net the W54 Phase 5 attempt
  needed.
- **Bench harness with σ < 5% from Phase 4 onward**, not
  retrofitted (W47).
- **Spec test infra integrated, not bolted on**. v1's
  `test/spec/run_spec.py` is Python; v2 might inline a Zig
  spec runner.

### 4.4 Knowledge-compression seeds

- **`docs/ja/learn_zwasm/NNNN_*.md` chapter series** paired
  with phases, paralleling CW v2's structure.
- **ADRs for every architectural choice**, including ones v1
  made implicitly (NaN-boxing not chosen, single-pass JIT
  chosen, two-arch maintained).
- **A "v1 retrospective" companion document** that reads v1's
  D100-D137 and synthesises what each decision would look like
  if redone. Useful both for v2 design and for v1 maintainers.

### 4.5 Scope seeds (negative space)

- **What does v2 not do?** v1's feature set may be
  over-broad for the user base. Candidates for explicit
  out-of-scope: GC proposal, exception handling, threads,
  multimemory — depending on whether ClojureWasm and the
  user's other use cases need them.
- **Which platforms?** v1 supports Mac aarch64, Linux
  x86_64/aarch64, Windows x86_64. v2 might tier these — a
  Windows port that nobody uses is maintenance debt.

These are intentionally framed as questions because the
investigation phase exists to answer them.

## 5. Hard constraints (if v2 happens)

These are non-negotiable framing rules, not phase-internal
choices:

- **v1 stays on `main`**. ClojureWasm depends on zwasm `main`
  via GitHub URL; v2 work cannot disrupt v1's release cadence.
- **v2 lives on a long-lived branch** (likely
  `develop/zwasm-v2`) or a separate worktree
  (`~/Documents/MyProducts/zwasm-from-scratch/`). The user
  approves all pushes from v2 to main; v1's main is otherwise
  untouched until v2 is ready to replace it (potentially
  years).
- **v2 has its own `.dev/ROADMAP.md`** as single-source-of-truth.
  v1's `.dev/roadmap.md` and decisions remain frozen reference.
- **English for code, comments, ADRs, ROADMAP**. Japanese for
  chat replies and `docs/ja/learn_zwasm/` chapters (mirroring
  CW v2's policy).
- **No premature merge.** v1 → v2 transition is a future event;
  v2 must reach feature-parity for ClojureWasm's actual usage
  before it becomes a candidate.

## 6. How to start (the next session's first day)

Per CW v2's working agreement, the first session's job is **not
to write code**. It is to:

### Step 1 — Read

- This charter, in full.
- `~/Documents/MyProducts/ClojureWasmFromScratch/CLAUDE.md` and
  `.dev/ROADMAP.md` (the entire 7000-line document, with focus
  on §1 mission, §2 principles, §4 architecture, §5 layout, §9
  phase plan).
- `~/Documents/MyProducts/ClojureWasmFromScratch/.dev/decisions/`
  (all ADRs).
- v1's `.dev/decisions.md` (D100-D137) — the synthesis target.
- v1's `.dev/w54-redesign-postmortem.md` — the most recent
  evidence of structural pain.
- Reference runtimes' high-level structure: cranelift
  (`~/Documents/OSS/wasmtime/cranelift/`), zware
  (`~/Documents/OSS/zware/`), and one of {wasmer, wasm3,
  wazero} for breadth.

### Step 2 — Investigate

- v1's pain points, layer by layer. `vm.zig`, `jit.zig`,
  `x86.zig`, `regalloc.zig`, `predecode.zig`, `c_api.zig`.
  Document what each does, what implicit contracts each holds,
  what would change if redesigned.
- Wasm spec status: which proposals are candidate-stable
  enough to design for, which are not.
- wasm-c-api header: what the standard ABI requires,
  what zwasm v1 exposes today, what the gap looks like.
- Industry runtimes' architecture: cranelift's CLIF → MIR →
  VCode → regalloc2 → emit pipeline; wasmer's singlepass vs
  cranelift backends; SpiderMonkey's WasmBaseline.
- Output: a 200-400 line `private/notes/v2-investigation.md`
  (gitignored) summarising findings.

### Step 3 — Charter the v2 ROADMAP

- Draft `~/Documents/MyProducts/zwasm-from-scratch/.dev/ROADMAP.md`
  (or in a `develop/zwasm-v2` worktree) on the CW v2 model:
  Mission / Inviolable principles / Architecture / Directory
  layout (final form) / Phase plan / Test strategy.
- The phase plan must answer: "what's Phase 1's exit criterion,
  what's Phase 2's, ... and where is the 🔒 x86_64 gate."
- Discussion seeds from §4 above either become Phase entries,
  ADRs, or out-of-scope notes — explicitly classify each.
- Output: a v2 ROADMAP draft, plus initial ADRs for any
  decision that diverges from v1's choices.

### Step 4 — Decide

- Re-read the v2 ROADMAP draft a day after writing it. Cut
  anything that smells like over-design.
- Decide whether v2 actually starts. The charter exists to
  make this decision possible, not to pre-commit it.

The user has explicitly said this v2 effort is **discussion-grade,
not commitment-grade**. The investigation phase is the
commitment-grade decision point. Sessions before that point
are research, not construction.

## 7. References

### Within this repo (v1)

- `.dev/decisions.md` — D100-D137, the v1 architectural
  decisions log
- `.dev/w54-redesign-postmortem.md` — most recent structural
  pain narrative; the bug class that motivated this charter
- `.dev/w54-investigation.md` — the W54 framing reset that
  preceded the redesign session
- `src/jit.zig`, `src/x86.zig`, `src/vm.zig` — the three
  hot files that bracket v1's complexity question
- `src/loop_info.zig` — the most recent example of
  "shared analysis layer" extracted from the two backends; v1's
  step toward what v2 would do natively

### ClojureWasm v2 (precedent)

- `~/Documents/MyProducts/ClojureWasmFromScratch/CLAUDE.md`
- `~/Documents/MyProducts/ClojureWasmFromScratch/README.md`
- `~/Documents/MyProducts/ClojureWasmFromScratch/.dev/ROADMAP.md`
- `~/Documents/MyProducts/ClojureWasmFromScratch/.dev/decisions/`
- `~/Documents/MyProducts/ClojureWasmFromScratch/.claude/`
  — the procedural toolkit (continue, audit_scaffolding,
  textbook_survey rules)

### External

- [wasm-c-api](https://github.com/WebAssembly/wasm-c-api) —
  industry-standard C ABI
- `~/Documents/OSS/wasmtime/cranelift/` — VCode +
  regalloc2 reference
- `~/Documents/OSS/zware/` — Zig idiom reference

## 8. Closing note

This charter is the artefact of a single session's discussion.
It is opinionated where v2's framing requires opinion, and
agnostic where the investigation phase has to answer the
question. **It is not a plan. It is a starting brief for a
deliberation that has not yet happened.**

The expected lifetime of this document: from first commit until
either (a) a v2 ROADMAP supersedes it, or (b) a follow-up
session decides v2 is not pursued and this charter is archived.
Either outcome is legitimate.
