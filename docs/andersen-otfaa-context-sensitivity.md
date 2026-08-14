# Opt-in context-sensitivity for AndersenOTFAA

> This document describes the *design*; where the implementation diverges
> from it, the divergence is noted inline. Two divergences are load-bearing
> for anyone tuning the feature: precision does not compose down the call
> chain at k = 1 (Section 5.3), and functions first reached as callbacks
> fall back to the root context (Section 5.5).

## 1. Problem

AndersenOTFAA (`AndersenOTFSolver::SolverData` in
`lib/PhasarLLVM/Pointer/AndersenOTFAA.cpp`, see Section 2) is
context-insensitive: `connectCallee` binds call actuals to a function's
formal parameters through **one shared node per formal parameter**,
reused by every call site of that function:

```cpp
const ValueId ParamId = getOrInsertVar(PAGVariable(&Param));
for (ValueId ArgId : ArgIds) { addAssignEdge(ArgId, ParamId); }
```

A parameter's points-to set is therefore the union over all callers, even
when those callers pass unrelated values. The same happens for heap/stack
objects: `getOrInsertObj` keys purely on the allocation-site
`llvm::Value*`, so two calls to a shared allocating helper produce one
merged object (unless caught by the narrow `isAllocWrapper` special case,
Section 6).

Concrete case, from analyzing the SPEC `spec-mesa` benchmark: a function
`draw()` calls `end(p, q)` from two call sites with different arguments.
Both calls bind into the same formal-parameter nodes for `end`, so `end`'s
two parameters become mutually may-alias inside `end`'s body — regardless
of how precisely any dispatch table or struct field was resolved to reach
that call. This document designs a fix: opt-in context-sensitivity, so
selected functions get one node per formal parameter **per calling
context** instead of one node total.

Field-sensitivity (the `FnPtrFieldWrites` mechanism, Section 2) and
context-sensitivity are orthogonal axes. `FnPtrFieldWrites` already
distinguishes *which field* of an object holds a function pointer; this
document addresses *which calling context* reaches a given call or object.

## 2. AndersenOTFAA today

Background needed to follow the rest of this document; skip if already
familiar with `lib/PhasarLLVM/Pointer/AndersenOTFAA.cpp`.

- **Node identity.** A PAG (pointer assignment graph) node is either an SSA
  pointer value or an abstract memory object, both represented as a
  `PAGVariable` (a tagged `llvm::Value*`). Every node is interned to a
  compact `ValueId` (a `uint32_t` strong typedef) via `LocalVC`, a
  `ValueCompressor<AndersenVar>` (`AndersenVar` = `PAGVariable` + an
  object/variable flag). `getOrInsertVar`/`getOrInsertObj` do the
  interning; `LocalVC.id2vars(Id)` maps a `ValueId` back to every
  `AndersenVar` merged into it.
- **Points-to sets and propagation.** Each `ValueId` has a `NodeInfo` (in
  the `Nodes` vector, indexed by `ValueId`) holding a `PtsSet`
  (`RawAliasSet<ValueId>`, a Roaring bitmap, see Section 7) and outgoing
  assignment edges. `addAssignEdge(Src, Dst)` records `pts(Src) ⊆
  pts(Dst)`; `propagate()` floods new pts-set members along edges to a
  local fixpoint. Nodes can also be merged outright via union-find
  (`SCCUf`/`merge()`/`rep()`) when a cycle collapses them.
- **Call resolution.** `resolveFPCall` (function-pointer calls),
  `resolveVtableCall` (virtual calls via a vtable pointer), and
  `resolveStructVCall` (calls loaded from a constant-struct field, or —
  via the `FnPtrFieldWrites` table — a heap/stack dispatch-table field
  with a provably-tracked write history) each iterate the caller-side
  pts-set and call `connectCallee` for every plausible target.
  `connectCallee` binds actuals to formals with `addAssignEdge`, as shown
  in Section 1 — one node per formal parameter, shared across all callers.
- **Deferred resolution.** A call or store that can't yet be resolved
  (its pts-set is still empty or growing) is recorded
  (`UnresolvedFPCalls`/`UnresolvedVCalls`/`UnresolvedStructVCalls`/
  `UnresolvedFieldWrites`) and retried every round.
- **Main loop.** `run()` drains a function worklist (each function's body
  visited once; direct calls enqueue their callee), then rechecks every
  `Unresolved*` record, looping
  `do { ... } while (!FunctionWorklist.empty() || Changed)` until nothing
  changes. This is a monotonic fixpoint: pts-sets and edges are only ever
  added, never retracted or shrunk — no operation in this solver removes
  anything once inserted.
- **`FnPtrFieldWrites`** (already implemented, sibling feature): tracks
  observed `store Function, GEP(base, const-indices)` writes per
  allocation site, giving precise resolution of function-pointer fields on
  heap/stack objects instead of today's field-insensitive collapse (every
  GEP result is unioned with its base pointer). Orthogonal to this
  document's topic; interaction covered in Section 6.

## 3. Background: context-sensitivity approaches

Context-sensitivity analyzes a function separately per calling context
instead of merging all callers into one node. Four established context
abstractions:

- **Call-string / k-CFA**: context = bounded stack of call sites
  (Sharir & Pnueli 1978; Shivers, *Control Flow Analysis in Scheme*, 1991).
  PHASAR's own `IDESolver` (`include/phasar/DataFlow/IfdsIde/Solver/IDESolver.h`)
  already implements exact call-string matching for IFDS/IDE via its
  exploded supergraph and summary functions — same idea, different
  (distributive-framework) formalism than Andersen's inclusion constraints.
- **Object-sensitivity**: context = allocation site of the receiver object
  (Milanova, Rountev, Ryder, *Parameterized Object Sensitivity*, TOSEM 2005).
  Built for OO receiver-dispatch; C has no receiver, so this maps weakly.
- **Type-sensitivity**: context = allocation site's *type*, k-limited
  (Smaragdakis, Bravenboer, Lhoták, *Pick Your Contexts Well*, POPL 2011).
  Also OO-specific; that paper's broader result — call-site sensitivity can
  be reshaped to dominate object-sensitivity once both are compared on
  equal footing (Li et al., *Return of CFA*, OOPSLA 2022) — favors
  call-string for a non-OO IR like LLVM.
- **CFL-reachability / demand-driven refinement**: exact context matching
  via balanced-parenthesis grammars over call/return edges, refined
  on-demand only where precision is needed (Sridharan & Bodík,
  *Refinement-based context-sensitive points-to analysis for Java*, PLDI
  2006; Sridharan et al., OOPSLA 2005).

Three lines of work address scaling context-sensitivity itself:

- **Heap cloning** — clone allocation sites per (acyclic) calling context,
  not just formal parameters. Lattner, Lenharth, Adve's *Data Structure
  Analysis* (PLDI 2007) does this for LLVM IR with a unification-based
  (Steensgaard-style) base analysis; Sui & Xue's *SUPA*/*ICON* line
  (staged, sparse, LLVM-based) does the inclusion-based (Andersen-style)
  analogue.
- **Selective context-sensitivity** — apply context-sensitivity only to a
  minority of "precision-critical" functions, context-insensitive
  elsewhere. Smaragdakis, Kastrinis, Balatsouras, *Introspective Analysis*
  (PLDI 2014), collapse expensive "legacy" contexts uniformly; Jeong et
  al., *Data-driven context-sensitivity* (OOPSLA 2017), learn a selection
  function from training programs; Li, Tan, Xue, *Zipper* /
  *Precision-Guided Context Sensitivity* (OOPSLA 2018; journal version
  TOPLAS 2020) identify precision-critical methods from static
  value-flow patterns, applying context-sensitivity to ~38% of methods
  while retaining ~99% of full context-sensitive precision.
- **Budgeted / graceful degradation** — cap total context-sensitive nodes
  and fall back soundly to context-insensitive treatment past the cap;
  standard practice in all production-scale implementations above.

## 4. Choice: k-limited call-string, selectively applied

Call-string context fits AndersenOTFAA best:

- The existing node-keying scheme (`AndersenVar`, Section 2) extends
  naturally to `(PAGVariable, ContextId)` for selected functions, without
  changing `AndersenVar`/`LocalVC` itself (Section 5.2) — no
  receiver-object concept needs inventing for a C/C++ IR, and no cost for
  functions that opt out.
- The solver already threads a call-site identity (`const llvm::CallBase
  *CS`) through `connectCallee`/`resolveFPCall`/`resolveVtableCall`/
  `resolveStructVCall`, so building call-strings from `CS` needs no new IR
  traversal.
- Object-sensitivity's advantage (linking a function's context to the
  object it operates on) is only useful here for the escaping-allocation-
  wrapper problem, already special-cased via `isAllocWrapper`. Full
  call-string sensitivity subsumes that special case for free (Section 6).

Apply it selectively (Zipper-style), not globally: most functions gain
nothing from context-sensitivity, and the goal is opt-in, bounded cost.

## 5. Design

### 5.1 Context representation

No new type is needed: `include/phasar/Pointer/CallingContextConstructor.h`
already provides `CallingContext<N, K>` (a `std::array<N, K>` of call sites,
newest first, whose `withPrefix()` *is* the k-limiting push) plus a
`CallingContextId` strong typedef whose `None` enumerator is id 0. Interning
is `Compressor<CallingContext<const llvm::CallBase *, 1>, CallingContextId>`.

The k-limit is a compile-time constant fixed at 1, so a context is one
pointer wide and `withPrefix(CS)` depends only on `CS`. This bounds the
context space to `O(NumCallSites)` and, combined with the truncation rule,
guarantees termination under recursion (Section 5.6).

`CallingContextId::None` denotes the context-insensitive root context —
every function starts here, matching today's behavior. With every function
routed to the root context, the design degenerates to exactly today's
solver: "context-sensitivity off" is a genuine zero-cost subset of "on",
not a separate code path.

### 5.2 Context-qualified PAG nodes

New key type, twice the size of `AndersenVar` (one pointer-sized word vs.
two):

```cpp
struct ContextualVar {
  AndersenVar Var;
  CallingContextId Ctx; // None for root-context nodes
};
```

Do **not** route every node through `ContextualVar`. `LocalVC` stays
exactly as-is and keeps handling every root-context node — same 1-word key,
same memory footprint, same `id2vars` cost as today. A second, separate
table holds only nodes belonging to *selected* functions (Section 5.4):

```cpp
class ContextualNodeTable {
  llvm::DenseMap<ContextualVar, ValueId> Var2Id;
  TypedVector<ValueId, llvm::SmallVector<ContextualVar, 1>> Id2Vars;
};
```

`getOrInsertVar`/`getOrInsertObj` branch on whether the value's owning
function is the selected function currently being translated: if not, use
the existing `LocalVC` path unchanged; if so, build a `ContextualVar` and
use the side table. Note that `ValueCompressor` has no `grow()` — ids for
contextual nodes are carved out of the *same* shared `ValueId` space with
`ValueCompressor::addDummy()`, so `PtsSet`/edges/worklist code stays
single-typed and doesn't care which table a `ValueId` came from. (This also
means `LocalVC.size()` remains the total node count, so `buildResult()`
needs no change to its bounds.)

One accessor, `forEachVar(Id, Fn)`, replaces the eight
`for (auto &Var : LocalVC.id2vars(Id))` scan loops: it visits `LocalVC`'s
list and then the side table's, since `addAlias` can merge both kinds of
name onto one id (a GEP inside a selected function aliased with a global's
node, say). With the feature off the side table is never resized, so this
costs one `inbounds` check.

This matters beyond interning cost: `id2vars(ObjId)` is rescanned every
outer fixpoint round inside `resolveStructVCall`/`resolveVtableCall`/
`resolveFieldWrite`, not just once during PAG construction. A single wider
key type for *all* nodes would double that recurring cost even with the
feature off. Splitting the tables makes "context-sensitivity off" (or "on
but this function wasn't selected") genuinely zero marginal cost, not just
an equivalent-result cost — `LocalVC` and its scans never see a
`ContextualVar`.

That "zero marginal cost" claim is about the **off** path only, and the
implementation bears it out: with `Mode::Off` the side table is never
resized. It does *not* extend to the on path, where the side table's
`Id2Vars` is not small. Contextual ids come from `LocalVC.addDummy()` and
are therefore interleaved with `LocalVC`'s own inserts, so
`recordVar`'s `Id2Vars.resize(size_t(Id) + 1)` grows the vector toward the
*total* node count — one mostly-empty
`SmallVector<ContextualVar, 1>` per node, contextual or not.

This is a deliberate trade, not an oversight: `forEachVar` is the hottest
accessor in the solver (per pts-element, per object, per fixpoint round),
and a contiguous `inbounds`-checked index is worth more there than the
memory a `DenseMap<ValueId, ...>` would save. Do not convert it without
measuring.

### 5.2a Function bodies are translated once per context

Cloning only formals, return slot and allocation sites is *not* enough: a
cloned parameter node whose assign-edges feed the shared body nodes
re-merges immediately, and the clone buys nothing. A selected function's
body must be re-translated once per context. Concretely:

- `FunctionWorklist`, `Queued` and `Processed` hold
  `std::pair<const llvm::Function *, CallingContextId>` instead of a bare
  function pointer.
- `processFunction(F, Ctx)` sets a `CurFunc`/`CurCtx` pair that
  `contextOf(Var)` consults: values of the function being translated are
  context-qualified, globals/constants/other functions' values are not.
- The `Unresolved*` records gain a `CallingContextId Ctx` field holding the
  *caller's* context, so re-resolution in later rounds reconstructs the
  same callee context.
- `ConnectedCallees` is keyed on `(CallBase *, CallerCtx)` and stores
  `(CalleeId, CalleeCtx)` pairs.

This is the source of the per-round record-count growth in Section 8.

Note that `Queued` alone already makes each `(F, Ctx)` pair reachable once:
every `FunctionWorklist` push is guarded by `Queued.insert(...).second`, so
`Processed` is a second source of truth that the implementation never
actually consults for a distinct answer. It costs a
`DenseSet<FuncCtx>` whose size scales with contexts, not just functions.

### 5.3 Context-sensitive call/return

`connectCallee` gains the caller's `ContextId` as a parameter (threaded
through from the call-site resolution functions, which already carry `CS`):

```cpp
bool connectCallee(const llvm::CallBase *CS, const llvm::Function *Callee,
                    ArgList Args, std::optional<ValueId> CSRetVal,
                    ContextId CallerCtx) {
  const ContextId CalleeCtx = isSelected(Callee)
      ? internContext(push(CallerCtx, CS))
      : ContextId{};
  ...
  const ValueId ParamId = getOrInsertVar(PAGVariable(&Param), CalleeCtx);
  ...
}
```

Return-value propagation must target the *caller's* context-qualified
return slot, not a shared one — otherwise return values re-merge across
contexts and erase the precision gain:

```cpp
const ValueId RetSlotId =
    getOrInsertVar(PAGVariable::Return{Callee}, CalleeCtx);
addAssignEdge(RetSlotId, *CSRetVal); // CSRetVal lives in CallerCtx already
```

This mirrors, at the constraint-graph level, the call/return matching
`IDESolver` already does via its exploded supergraph (Section 3) — the
call-string analogue for an inclusion-constraint solver instead of a
summary-function solver.

Allocation sites inside a selected function are cloned the same way:
`getOrInsertObj(PAGVariable(AllocSite), CalleeCtx)`. This directly
generalizes `isAllocWrapper` (Section 6).

**Precision does not compose down the call chain at k = 1.** `pushContext`
takes the caller's context, but `CallingContext<N, 1>::withPrefix` discards
the existing frame, so the resulting context depends on the call site
alone. Contexts are in bijection with call sites, and selecting a caller
buys its callees nothing:

- `F` is selected and cloned into `F@C1` and `F@C2`.
- Both clones call `G` at the same call site `CS`.
- `calleeContext(G, C1, CS)` and `calleeContext(G, C2, CS)` both yield
  `{CS}`, so both clones bind their actuals into the *same* `G@{CS}`
  formals.
- `G`'s return slot then flows the re-merged set back to the call-site
  nodes in both `C1` and `C2`.

So the precision gain is exactly one call-site frame deep, at the selected
function itself. Selecting a whole call chain via `AllowList` does not
deepen it — only raising k would, and Section 11's first open question
explains why that is not a free knob (`MaxContextsPerFunction` would bind
almost immediately, paying k = 2 costs for k = 1 precision on hot
functions). Tune `AllowList` on the assumption that the function you name
is the *only* one that gains.

### 5.4 Selection ("opt-in")

A single `SelectionMode` enum, coarsest to finest:

- **`Off`** (default) — root-context-only path; zero behavior/perf change
  from today.
- **`Manual`** — only functions matching an allow-list of function-name
  globs (`llvm::GlobPattern`), for users who already know which function
  needs precision (e.g. `end` in the `spec-mesa` case).
- **`Dynamic`** — the allow-list plus functions matching a syntactic
  precision-critical test (below).
- **`All`** — every function, until the node budget is reached.

A deny-list of globs is checked first in every mode and always wins.

**Dynamic selection is a syntactic test, decided before any wiring.** It
does not ask "do the callers pass different values" -- undecidable up front
-- but "if they do, can anyone tell?" A function qualifies if it has
several call sites (two direct `CallBase` users, or address-taken) and one
of:

- **Strong: something passed in leaves again.** A param-derived value is
  returned, stored through a global or param-derived pointer, or used as
  the callee of an indirect call -- polluting callers, the heap, or the
  call graph respectively. These are the syntactic counterparts of Zipper's
  precision-loss patterns (Li, Tan, Xue, OOPSLA 2018) and are what
  generalizes.
- **Weak: two or more pointer parameters, nothing escaping.** The merge can
  then only make the formals alias *within* the body -- the `end(p, q)`
  case of Section 1, which returns void and dispatches nothing. Common
  enough on C++ (`this` plus one pointer argument matches most methods),
  so it is gated on the much tighter `MaxLocalMergeFunctionSize`, where a
  clone is nearly free. **Off by default** (`0`) since Section 7.1: the
  tier is measurably inert, because the aliasing it recovers is exactly
  what `buildResult` unions back together across contexts.

"Param-derived" is a backward def-use walk (casts / GEPs / loads / phis /
selects, continuing through values stored into a local alloca to cover
un-`mem2reg`'d parameters). All strong patterns are checked in one pass
sharing one cache, so the test is linear in function size; the cache
memoizes negative answers only and may under-approximate through
loop-carried cycles, which costs a missed selection, never soundness.

Both tiers are capped by `MaxContextualFunctionSize` (256 instructions):
a selected function costs one clone of its *entire body* per context
(Section 5.2a), so body size dominates the cost.

Two budgets bound the cost, both sound (strictly less precise, never
incorrect):

- **`MaxContextsPerFunction`** (default 32) caps how many contexts one
  function may be cloned into. Past it, further call sites fall back to the
  shared root context. Without it a single function called from 400 sites
  costs 400 body clones. It also answers what was open question 2.
- **`MaxContextualNodes`** (default 20k) caps context-qualified nodes
  globally. Once reached, no *further* function is selected **and no
  already-selected function gets a further context**. The second half is
  what makes it a cost governor at all: selection is decided and memoized
  on first encounter, before any clone of that function exists, so gating
  selection alone lets the functions selected in the first few rounds keep
  minting contexts arbitrarily far past the budget (Section 7.1).

Because selection is decided and cached on first query, and the solver's
traversal order is deterministic, which functions fit inside the budgets is
reproducible run-to-run. It is *not* value-ordered, though: on an input
large enough to exhaust `MaxContextualNodes`, the functions reached first
win rather than the most precision-critical ones. Ranking candidates before
admitting them would need a whole-module pre-pass; both predicates are
purely syntactic, so that is a possible refinement, not a redesign.

These options (plus the fixed k-limit, Section 5.1) are new, user-facing
configuration in a `ContextSensitivityOptions` struct, threaded through
`AndersenOTFSolver`'s constructor and the `computeAndersenOTFRaw`/
`computeAndersenOTF` factory functions (`include/phasar/PhasarLLVM/Pointer/AndersenOTFAA.h`),
alongside the existing `Soundness` parameter.

### 5.5 Selection must be decided before a function is first wired

An earlier draft of this document proposed promoting a function *mid-solve*,
the moment the resolvers observed it dispatching to several targets, on the
grounds that a freshly cloned node starts empty and so cannot inherit merged
facts. That reasoning is correct about the *cloned* node and wrong about the
result, because it ignores the caller side:

1. `main` calls `end` twice. `connectCallee` wires
   `Return{end}@root -> xx` and `-> yy` and propagates.
2. `end` is then processed, its dispatch resolves to two targets, and `end`
   is flagged as precision-critical.
3. Promotion adds `Return{end}@ctx22 -> xx`. But the round-1 edge and
   everything it already propagated stay: nothing in this solver retracts an
   edge or shrinks a pts-set (Section 2). `xx` keeps the merged
   `{x, y}` forever and the promotion buys nothing.

Measured on `test/llvm_test_code/pointers/context_15.c`: mid-solve promotion
leaves `xx` and `yy` aliasing; selecting the same single function `end` up
front separates them completely. So selection is decided on the *first*
`isSelected` query for a function and cached from then on — before any of
its nodes exist. That is what makes the syntactic test in Section 5.4 the
right shape of detector: it needs no solver state, so it can answer early
enough to matter.

Consequently there is no promotion event, no mid-solve restart, and no
extra convergence round. `run()`'s `do { ... } while (...)` loop is
unchanged except for the worklist element type (Section 5.3).

**Exception: callback-reachable functions.** `addFnPtrArgsAsEntries` queues
every function that reaches a declaration's fn-ptr argument as a new entry
point at `CallingContextId::None`, without consulting `isSelected`. A
selected function that is *first* reached this way therefore has its root
clone wired before any contextual clone exists — structurally the same
situation this section argues against, and with the same consequence: what
the root clone already propagated stays propagated, so the contextual
clones created later by direct call sites recover less than they would
have.

This is inherent, not an oversight. A callback has no known call site by
construction, so there is no call string to push and the root context is
the only sound answer available. Minting a synthetic context per
callback-introducing call site would recover the precision but changes the
context domain from "call site" to "call site or callback origin" and
burns `MaxContextsPerFunction` on sites that share no useful structure —
not worth it.

Practical consequence: a function that is both called directly and passed
as a callback keeps a merged root clone *alongside* its contextual clones,
and `buildResult` unions the two (Section 5.3's note on shared external
ids). Expect selection to under-deliver on exactly the callback-heavy C
idioms — `qsort`-style comparators, dispatch tables handed to library
code — that `Mode::Dynamic` is otherwise most likely to pick.

### 5.6 Soundness and termination

- **Truncation is sound, only imprecise**: collapsing context strings once
  they exceed the k-limit (or repeat a frame — recursion) merges facts from
  distinct realizable paths into one node, which can only *add* pts-set
  members relative to unbounded call-strings, never drop sound facts. This
  is the standard k-CFA soundness argument (Shivers 1991; Sharir & Pnueli 1978)
  and needs no new proof obligation.
- **Termination under recursion**: the fixed-size `CallingContext` array
  bounds `CallingContextId` to a finite set (`NumCallSites` at k = 1), so
  the `ContextualVar` domain is finite and `run()`'s existing monotonic
  fixpoint argument (Section 2: pts-sets and edge sets only grow,
  `Changed`-driven convergence) carries over unchanged — recursion just
  means some contexts get reused (revisited) rather than growing the
  domain further.
- **Monotonicity**: pts-sets, edges and call-graph edges are still only
  ever added, never retracted. Selection adds no new kind of event: it is
  fixed per function before that function's first node exists (Section 5.5),
  so the `do { ... } while (...)` architecture in `run()` needs no
  structural change; `checkUnresolvedX`-style re-resolution passes just
  operate over context-qualified keys where relevant.
- `isSelected(F)` is **memoized**, so a function is never re-decided and
  never re-cloned under a changed verdict.

## 6. Interaction with existing mechanisms

- **`isAllocWrapper`**: today's special case gives each call site of an
  alloc-wrapper its own object via `getOrInsertObj(PAGVariable(CS))` keyed
  on the call site itself — a hand-rolled, unconditional 1-context clone.
  Once general call-string object cloning exists, this becomes a special
  case of "wrapper function selected for context-sensitivity"; the two can
  coexist during rollout, but the special case becomes removable once
  dynamic selection (Section 5.4) covers alloc wrappers by default (they
  trivially trigger it: multiple call sites, object flows into
  precision-critical resolution). It is kept for now: it also covers
  wrappers in `Off`/`Manual` mode, where no selection applies.
- **`FnPtrFieldWrites`** (Section 2): orthogonal and composable.
  Field-sensitivity resolves *which field* holds a function pointer;
  context-sensitivity resolves *which object* (or *which parameter
  binding*) a given call actually reaches. Combining both means a
  context-cloned heap object gets its own `FnPtrFieldWrites`/
  `ImpureObjects` entries too. Implemented: the shared
  `ObjectKey { const llvm::Value *Val; CallingContextId Ctx; }` now keys
  `FnPtrFieldWrites` (via `FieldWriteKey`), `FieldsByObject` and
  `ImpureObjects`; the context comes straight off the `ContextualVar` that
  `forEachVar` yields for the object node, so no extra plumbing is needed.
  Both overloads must resolve their recorded pointer through `rep()` before
  reading `PtsSet`: once that pointer is collapsed into an SCC its
  `NodeInfo` is cleared, so a non-representative reads empty and nothing
  gets poisoned.
- **`resolveStructVCall`/`resolveFPCall`/`resolveVtableCall`**: unaffected
  in structure; they already snapshot `PtsSet` by value/reference and loop
  per-object — context only changes what a "formal parameter" or "object"
  *is* (a context-qualified node instead of a bare one), not how these
  functions traverse pts-sets.

## 7. Scalability controls (summary)

| Control | Default | Effect |
|---|---|---|
| `SelectionMode` | `Off` | Root-context-only; identical to today |
| k-limit | 1 (compile-time) | Call-string depth; bounds context count per function |
| `SelectionMode::Dynamic` | -- | Scopes cloning to syntactically precision-critical functions |
| `AllowList` / `DenyList` | empty | User override; deny always wins |
| `MaxContextsPerFunction` | 32 | Per-function clone cap; extra call sites fall back to root |
| `MaxContextualFunctionSize` | 256 insts | `Dynamic` only: skips functions too big to clone |
| `MaxLocalMergeFunctionSize` | 0 (off) | `Dynamic` only: tighter cap for the weak signal |
| `MaxContextualNodes` | 20k | Global cost governor: past it, no further function is selected and no selected function gets a further context. `AllowList` matches are exempt from the selection half (see below) |

`MaxContextualNodes` is not an absolute ceiling. `computeIsSelected` tests
`DenyList`, then `AllowList`, and only then consults `budgetExhausted()`,
so an allow-listed function is selected however much budget is already
spent. That is deliberate: silently ignoring an explicit user request
because an unrelated function got there first would be worse than
overshooting the budget, and the outcome would depend on function
processing order. The cap governs what `Mode::Dynamic`/`Mode::All` infer on
their own. Size an `AllowList` accordingly — it is a commitment, not a
request.

### 7.1 How the defaults were chosen (measured)

The original constants were tuned on coreutils alone and did not transfer:
`Dynamic` recovered ~100% of `Mode::All`'s precision there but only 31% on
`readelf` and 34% on `lrzip`, while costing 6.9x on `bison` for a 1.0%
gain. Re-tuned against six programs from the `ir-15` corpus, release build,
entry point `main`; precision is total alias entries (sum of alias-set
sizes over all external values), lower is better.

| program | insts | `Off` | old defaults | **new defaults** | `All` |
|---|---|---|---|---|---|
| bison | 119k | 83.594M / 1.17s | 82.764M / 7.99s | 82.764M / **3.46s** | 82.764M / 10.7s |
| readelf | 103k | 38.443M / 0.54s | 37.242M / 0.59s | **24.627M** / 0.56s | 34.537M / 1.41s |
| lrzip | 77k | 12.887M / 0.45s | 12.840M / 1.20s | **12.708M** / 2.10s | 12.748M / 1.37s |
| mjs | 38k | 1.2364M / 0.05s | 1.2306M / 0.09s | **1.2225M** / 0.13s | 1.2306M / 0.09s |
| cxxfilt | 336k | 9.010M / 0.16s | 8.104M / 0.32s | 8.104M / 0.38s | 8.104M / 0.56s |
| lepton | 233k | 232.30M / 2.11s | 227.88M / 4.88s | 231.58M / **3.44s** | 227.76M / 15.4s |

What each change is buying:

- **`MaxContextsPerFunction` 8 -> 32** is the precision change. The cap was
  binding constantly on exactly the functions worth cloning. `readelf` has
  a cliff between 24 and 28 contexts: 35.23M at 24, 24.84M at 28. Below the
  cliff the analysis pays for 8 clones of a hot function *and still* merges
  its remaining callers into the root clone -- the worst of both. `lrzip`,
  `mjs` and `cxxfilt` improve as well; `bison` is indifferent.
- **`MaxContextualNodes` 200k -> 20k** is the cost change, and only works
  together with the `calleeContext` half of the check. Peak usage was 89k
  nodes (`bison`, at 32 contexts) and 10-21k everywhere else, so the old
  value could never bind. Raising the context cap alone puts `bison` at
  42s; with the budget it is 3.46s -- *faster than the old defaults* -- at
  identical precision, because every `bison` context past ~20k nodes bought
  0.0008%. Time is super-linear in the node count: `bison` goes 2.6s / 3.5s
  / 10.7s / 24.4s at budgets of 16k / 20k / 24k / 32k.
- **`MaxLocalMergeFunctionSize` 32 -> 0** is free. Across every
  (contexts, size, budget) combination tried, values of 0, 32 and 256
  produced byte-identical alias counts on all six programs. The tier cannot
  pay while `buildResult` unions a formal's clones back into one external
  id, which is precisely the aliasing it is meant to separate. Turning it
  off drops a body scan; it becomes worth re-enabling only if that
  projection is fixed.
- **`MaxContextualFunctionSize` stays 256.** 1024 buys `readelf` and
  `lrzip` a little more and costs `bison` ~20%; 64 loses `readelf`'s cliff
  entirely.

Known trade, not papered over: **`lepton` is worse than before** (231.58M
vs 227.88M, though 1.4x faster). It is the one program that wants a *large*
budget -- at 32k it reaches 225.92M, beating `Mode::All`, but 32k costs
`bison` 24.4s. No single global constant satisfies both, because the budget
is absolute while the useful amount scales with program size. A
size-proportional budget does not fix it either (`bison` tolerates 0.19
nodes/instruction, `lrzip` wants 0.27). Callers that care about a specific
large program should raise `MaxContextualNodes` explicitly.

Caveat on all of the above: alias-entry count is a proxy for precision, not
a ground-truth comparison, and six programs is still a small corpus. The
`ptaben` ground-truth queries remain the check that matters.

## 8. Expected regressions when the feature is used

Section 5.2's table split makes the *off* path free (Section 7). These
costs are inherent to actually *using* the feature — unavoidable, but
should be sized/tested for, not discovered later:

- **`RawAliasSet<ValueId>` is unaffected.** Checked against the actual
  implementation (`include/phasar/Pointer/RawAliasSet.h`): it is a
  Roaring bitmap (`RoaringAliasSet`), not a fixed-width bitvector. Adding
  many new `ValueId`s from context cloning does not inflate the memory of
  *unrelated*, already-existing pts-sets just because the `ValueId` domain
  got larger — Roaring is sparse/compressed.
- **Recurring re-scan cost grows with record count, not just node count.**
  `checkUnresolvedFPCalls`/`checkUnresolvedVCalls`/
  `checkUnresolvedStructVCalls`/`checkUnresolvedFieldWrites` each rescan
  their *entire* vector every outer round (Section 2). Context-cloning a
  call/store site inside a selected function multiplies its record count
  by however many contexts reach it — a genuine per-round algorithmic cost
  increase proportional to selection aggressiveness, not fixed by the
  table split; needs its own benchmark, not just a memory argument.
- **Selected function bodies are re-translated per context** (Section
  5.2a). This is where most of the added work lives: `processFunction` runs
  once per `(Function, ContextId)` pair, and every instruction it visits
  creates its own contextual node.
- **Dynamic selection costs one syntactic scan per function.** The
  `hasMultipleCallSites` + `dispatchesThroughParam` test (Section 5.4) is
  a linear walk over the function's instructions plus a bounded def-use
  walk, run lazily once and memoized -- not once per round. Allow/deny
  lists skip it for functions the user already knows about.
- **`FnPtrFieldWrites`/`ImpureObjects` inherit the same per-round re-scan
  cost** now that they are keyed on `ObjectKey` (Section 6) --
  `resolveFieldWrite`/`mergeFieldWriteInfo` also re-run every round, so
  this table is subject to the identical growth pattern.
- **`MaxContextualNodes` cutoff order must be deterministic** (Section
  5.4). It is: selection is decided on first query in the solver's own
  deterministic traversal order and memoized, never by iteration order of
  a `DenseSet`/`DenseMap`. Otherwise two runs over the same input could
  select different subsets and produce different (each individually sound)
  precision -- a reproducibility regression, not a soundness one, but
  still surprising to a user re-running the same command.

## 9. Implementation plan

1. Reuse `CallingContext`/`CallingContextId`/`Compressor` from
   `include/phasar/Pointer/CallingContextConstructor.h` (Section 5.1).
2. `ContextualVar` + `ContextualNodeTable`; `getOrInsertVar`/
   `getOrInsertObj` overloads taking a `CallingContextId`; replace the
   `LocalVC.id2vars` scan loops with `forEachVar`. Verify the entire
   existing test suite is unaffected with the feature off (a no-op change
   at this step).
3. Thread `CallingContextId` through `connectCallee` and its callers
   (`resolveFPCall`, `resolveVtableCall`, `resolveStructVCall`,
   `handleCall`, entry-point setup) and make the worklist/`Queued`/
   `Processed`/`Unresolved*` records context-qualified (Section 5.2a).
   Still off by default.
4. `ContextSensitivityOptions` (Section 5.4): `SelectionMode`, allow/deny
   globs, `MaxContextualNodes`. Thread through `AndersenOTFSolver`'s
   constructor and the `computeAndersenOTFRaw`/`computeAndersenOTF`
   factory functions.
5. `isSelected` with the syntactic precision-critical test and the node
   budget (Sections 5.4, 5.5).
6. Extend the field-write tables to `ObjectKey` (Section 6).
7. Tests (new cases in `unittests/PhasarLLVM/Pointer/AndersenOTFAATest.cpp`,
   fixtures under `test/llvm_test_code/pointers/`, following the existing
   `AndersenOTFAATest` conventions — `computeAndersenOTFRaw` +
   `Res.CG.getCalleesOfCallAt(CS)` + `EXPECT_TRUE`/`EXPECT_FALSE
   (llvm::is_contained(...))`):
   - Precision positive case mirroring the `end()` example (Section 1):
     two call sites, distinct arguments, assert the two call results stop
     aliasing under `Dynamic`/`All`, where the `Off` baseline gives
     `MayAlias`.
   - Precision in the `test/llvm_test_code/pointers/context_*` examples.
   - Recursion termination: self-recursive and mutually-recursive
     functions selected; solver still terminates and stays at least as
     sound as the `Off` baseline.
   - Budget-exceeded graceful degradation: artificially tiny
     `MaxContextualNodes`; result still matches the flag-off baseline, no
     crash or incorrect drop.
   - `FnPtrFieldWrites` + context cloning interaction (Section 6):
     dispatch table allocated inside a shared helper called from two
     contexts; assert no cross-context contamination.
   - Full existing `AndersenOTFAATest` suite unchanged with the flag off.
8. Benchmark: `ptaben` runs every configuration side by side --
   `AndersOTF` (off), `AndersOTFCtxDyn` and `AndersOTFCtxAll` are separate
   analysis types with their own results CSV, so precision and cost can be
   diffed directly. The `end()` query (Section 1) should flip from
   `MayAlias` to the ground-truth-matching result.

## 10. Alternatives considered

- **Object-sensitivity**: rejected as primary abstraction — no natural
  receiver concept in C: an allocation site is already effectively "the
  object," so object-sensitivity would collapse to allocation-site
  context, a subset of what call-string + object cloning already gives,
  for extra conceptual complexity.
- **Full (unbounded) CFL-reachability**: most precise, but demand-driven
  CFL solvers are a different algorithmic family from the current
  worklist/union-find inclusion solver; adopting it means rewriting the
  solver core rather than extending it, and it lacks an obvious "opt-in /
  partial" mode the way selective call-string cloning has. Worth
  revisiting only if selective call-string proves insufficient in
  practice.
- **Always-on global context-sensitivity**: rejected outright — conflicts
  with the "opt-in" requirement and with AndersenOTFAA's own design goal
  of staying cheap enough to run on-the-fly during call-graph
  construction.

## 11. Open questions

1. The k-limit is fixed at 1 at compile time: cheapest, and it already
   fixes the `end()` pattern (one call-site frame distinguishes `draw`'s
   two calls). Whether deeper call chains need k = 2 in practice is an
   empirical question for the benchmark suite; raising it means changing
   the `CallingContext` template argument and making the frame count a
   runtime parameter.
2. Whether budget admission should be value-ordered rather than
   first-reached (Section 5.4). Only matters on inputs big enough to
   exhaust `MaxContextualNodes`; `MaxContextsPerFunction` already removes
   the worst case, one hot function starving everything else.
3. Whether the dynamic test (Section 5.4) should also cover the
   *non-dispatching* form of the problem — a function whose two pointer
   parameters become mutually may-alias without any indirect call in
   between. The current test deliberately does not, because "several call
   sites and several pointer parameters" matches far too many functions to
   be a useful selector.
4. Whether `isAllocWrapper` can be dropped once `Dynamic` mode is the
   default (Section 6). It is currently kept because `Off`/`Manual` mode
   still relies on it.

## 12. References

- Sharir, Pnueli. *Two Approaches to Interprocedural Data Flow Analysis*. 1978.
- Shivers. *Control Flow Analysis in Scheme*. PLDI 1988 / PhD thesis 1991.
- Milanova, Rountev, Ryder. *Parameterized Object Sensitivity for Points-to
  Analysis for Java*. TOSEM 2005.
- Sridharan, Gopan, Shan, Bodík. *Demand-Driven Points-to Analysis for
  Java*. OOPSLA 2005.
- Sridharan, Bodík. *Refinement-Based Context-Sensitive Points-To Analysis
  for Java*. PLDI 2006.
- Lattner, Lenharth, Adve. *Making Context-Sensitive Points-to Analysis
  with Heap Cloning Practical for the Real World*. PLDI 2007.
- Smaragdakis, Bravenboer, Lhoták. *Pick Your Contexts Well: Understanding
  Object-Sensitivity*. POPL 2011.
- Kastrinis, Smaragdakis. *Hybrid Context-Sensitivity for Points-To
  Analysis*. PLDI 2013.
- Smaragdakis, Kastrinis, Balatsouras. *Introspective Analysis:
  Context-Sensitivity, Across the Board*. PLDI 2014.
- Sui, Ye, Xue et al. *SUPA* / *ICON*: staged, sparse, context-sensitive
  Andersen-style analysis for LLVM IR.
- Jeong, Kim, Kim, Oh. *Data-Driven Context-Sensitivity for Points-to
  Analysis*. OOPSLA 2017.
- Li, Tan, Xue. *Precision-Guided Context Sensitivity for Pointer
  Analysis* ("Zipper"). OOPSLA 2018; journal version (with ZipperE):
  *A Principled Approach to Selective Context Sensitivity for Pointer
  Analysis*, TOPLAS 2020.
- Li et al. *Return of CFA: Call-Site Sensitivity Can Be Superior to
  Object Sensitivity Even for Object-Oriented Programs*. OOPSLA 2022.
