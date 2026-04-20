/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include "phasar/ControlFlow/CFG.h"
#include "phasar/ControlFlow/ICFG.h"
#include "phasar/DataFlow/WPDS/PAutomaton.h"
#include "phasar/DataFlow/WPDS/Semiring.h"
#include "phasar/DataFlow/WPDS/WPDS.h"
#include "phasar/DataFlow/WPDS/WPDSIds.h"
#include "phasar/DataFlow/WPDS/WPDSProblem.h"
#include "phasar/DataFlow/WPDS/WPDSRule.h"
#include "phasar/DataFlow/WPDS/WPDSSolverResults.h"
#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstdint>
#include <deque>
#include <utility>

namespace psr {
namespace wpds {

/// Solver for a WPDS-based interprocedural dataflow problem.
///
/// Implements the post* saturation algorithm (Algorithm 3, Fig. 17) from:
///   Reps, Schwoon, Jha, Melski, "Weighted Pushdown Systems and their
///   Application to Interprocedural Dataflow Analysis",
///   SciCP 58 (2005), DOI: 10.1016/j.scico.2005.02.009
///
/// Uses the Section 4.3 exploded-supergraph encoding:
///   control locations = dataflow facts (d_t), compressed to LocId
///   stack symbols     = program nodes  (n_t), compressed to SymId
///
/// \tparam ProblemTy A type satisfying WPDSProblem<AnalysisDomainTy>.
/// \tparam AnalysisDomainTy A type satisfying WPDSAnalysisDomain.
template <typename ProblemTy, typename AnalysisDomainTy>
  requires WPDSProblem<ProblemTy, AnalysisDomainTy> &&
           psr::CFG<typename AnalysisDomainTy::i_t> &&
           psr::ICFG<typename AnalysisDomainTy::i_t>
class WPDSSolver {
  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using w_t = typename AnalysisDomainTy::w_t;
  using i_t = typename AnalysisDomainTy::i_t;

  using AutomatonTy = PAutomaton<w_t>;
  using Transition = typename AutomatonTy::Transition;

public:
  explicit WPDSSolver(ProblemTy &Problem) : Problem(Problem) {}

  /// Run the full analysis: build WPDS, saturate, extract node values.
  void solve() {
    buildWPDS();
    buildInitialAutomaton();
    saturate();
    computeNodeValues();
  }

  /// Returns the saturated P-automaton and per-(fact,node) values.
  [[nodiscard]] WPDSSolverResults<w_t> getResults() && {
    return WPDSSolverResults<w_t>(std::move(Aut), std::move(NodeValues));
  }

  /// Returns the compressed LocId of a dataflow fact (control location).
  [[nodiscard]] LocId factId(const d_t &D) const { return FactComp.get(D); }

  /// Returns the compressed SymId of a program node (stack symbol).
  [[nodiscard]] SymId symId(n_t N) const { return NodeComp.get(N); }

  /// Returns the LocId for a fact, or nullopt if not discovered during build.
  [[nodiscard]] std::optional<LocId> factIdOr(const d_t &D) const noexcept {
    return FactComp.getOrNull(D);
  }

  /// Returns the SymId for a node, or nullopt if not discovered during build.
  [[nodiscard]] std::optional<SymId> symIdOr(n_t N) const noexcept {
    return NodeComp.getOrNull(N);
  }

  /// Returns the computed value for (fact, node) without consuming the solver.
  [[nodiscard]] w_t getNodeValue(LocId Fact, SymId Sym) const noexcept {
    auto It = NodeValues.find({Fact, Sym});
    return It != NodeValues.end() ? It->second : w_t::zero();
  }

  /// Direct access to the node-value map for iteration without a move.
  [[nodiscard]] const typename WPDSSolverResults<w_t>::NodeValueMap &
  getNodeValueMap() const noexcept {
    return NodeValues;
  }

  /// Reverse lookup: fact at compressed id.
  [[nodiscard]] decltype(auto) factAt(LocId L) const { return FactComp[L]; }
  /// Reverse lookup: node at compressed id.
  [[nodiscard]] decltype(auto) nodeAt(SymId S) const { return NodeComp[S]; }

private:
  // ─── Phase 1: Build WPDS from ICFG ─────────────────────────────────────────

  void buildWPDS() {
    const i_t &ICFG = Problem.getICFG();

    // Exploded-supergraph worklist: only visit (fact, node) pairs where the
    // fact can actually hold at that node.
    using FactNodePair = std::pair<LocId, SymId>;
    std::deque<FactNodePair> FNWorklist;
    llvm::DenseSet<FactNodePair> FNSeen;

    const auto ScheduleIfNew = [&](LocId DIdx, SymId NSym) {
      if (FNSeen.insert({DIdx, NSym}).second) {
        FNWorklist.emplace_back(DIdx, NSym);
      }
    };

    // Seed: zero fact at each entry point.
    LocId ZeroIdx = FactComp.getOrInsert(Problem.getZeroFact());
    assert(ZeroIdx == LocId{} && "The zero-value should be 0");
    for (auto E : Problem.getEntryPoints()) {
      ScheduleIfNew(ZeroIdx, NodeComp.getOrInsert(E));
    }

    while (!FNWorklist.empty()) {
      auto [DIdx, NSym] = FNWorklist.front();
      FNWorklist.pop_front();
      const d_t &D = FactComp[DIdx];
      n_t N = NodeComp[NSym];

      if (ICFG.isCallSite(N)) {
        for (auto R : ICFG.getSuccsOf(N)) {
          SymId RSym = NodeComp.getOrInsert(R);

          for (auto Callee : ICFG.getCalleesOfCallAt(N)) {
            for (auto E : ICFG.getStartPointsOf(Callee)) {
              SymId ESym = NodeComp.getOrInsert(E);
              for (auto [Dprime, W] : Problem.callFlowWeights(N, E, R, D)) {
                LocId DPrimeIdx = FactComp.getOrInsert(Dprime);
                Sys.addPushRule(DIdx, NSym, DPrimeIdx, ESym, RSym,
                                std::move(W));
                ScheduleIfNew(DPrimeIdx, ESym);
              }
            }
          }

          for (auto [Dprime, W] : Problem.callToReturnFlowWeights(N, R, D)) {
            LocId DPrimeIdx = FactComp.getOrInsert(Dprime);
            Sys.addInternalRule(DIdx, NSym, DPrimeIdx, RSym, std::move(W));
            ScheduleIfNew(DPrimeIdx, RSym);
          }
        }

      } else if (ICFG.isExitInst(N)) {
        auto Fun = ICFG.getFunctionOf(N);
        for (auto [Dprime, W] : Problem.returnFlowWeights(N, D)) {
          LocId DPrimeIdx = FactComp.getOrInsert(Dprime);
          Sys.addPopRule(DIdx, NSym, DPrimeIdx, std::move(W));
          // Schedule D' at every return site of every call to this function.
          for (auto Caller : ICFG.getCallersOf(Fun)) {
            for (auto R : ICFG.getSuccsOf(Caller)) {
              ScheduleIfNew(DPrimeIdx, NodeComp.getOrInsert(R));
            }
          }
        }

      } else {
        for (auto Succ : ICFG.getSuccsOf(N)) {
          SymId SuccSym = NodeComp.getOrInsert(Succ);
          for (auto [Dprime, W] : Problem.intraFlowWeights(N, Succ, D)) {
            LocId DPrimeIdx = FactComp.getOrInsert(Dprime);
            Sys.addInternalRule(DIdx, NSym, DPrimeIdx, SuccSym, std::move(W));
            ScheduleIfNew(DPrimeIdx, SuccSym);
          }
        }
      }
    }
  }

  // ─── Phase 2: Build initial P-automaton ────────────────────────────────────

  void buildInitialAutomaton() {
    const auto NumFacts = static_cast<uint32_t>(FactComp.size());

    // Allocate one automaton state per control location (= fact).
    // StateId(to_underlying(L)) == L for all LocId L — the layout is shared.
    for (uint32_t I = 0; I < NumFacts; ++I) {
      StateId S = Aut.addState();
      assert(to_underlying(S) == I && "State IDs must match fact IDs");
      Aut.markInitial(S);
    }

    // Single accepting state q_f.
    QFinal = Aut.addState();
    Aut.markFinal(QFinal);

    // Phase I of Algorithm 3: pre-create witness states for push rules.
    for (const auto &Rule : Sys.getAllRules()) {
      if (Rule.Kind == WPDSRuleKind::Push) {
        Aut.getOrCreateWitnessState(Rule.ToLoc, Rule.ToSym1);
      }
    }

    // Initial transitions for each entry point of the analysis.
    LocId ZeroLoc = FactComp.get(Problem.getZeroFact());
    for (auto E : Problem.getEntryPoints()) {
      SymId ESym = NodeComp.getOrInsert(E);
      w_t InitW = Problem.getInitialWeight(E);
      bool Changed = Aut.update(toStateId(ZeroLoc), ESym, QFinal, InitW);
      if (Changed) {
        Worklist.push_back({toStateId(ZeroLoc), ESym, QFinal});
      }
    }
  }

  // ─── Phase 3: Algorithm 3 — post* saturation ───────────────────────────────

  void saturate() {
    while (!Worklist.empty()) {
      Transition T = Worklist.front();
      Worklist.pop_front();

      if (T.Sym == kEpsilonSym) {
        processEpsilonTransition(T);
      } else {
        processNonEpsilonTransition(T);
      }
    }
  }

  void processNonEpsilonTransition(const Transition &T) {
    // WPDS rules are indexed by control locations (initial states). Witness
    // states (return continuations) have no rules — skip them.
    if (!Aut.isInitial(T.From)) {
      return;
    }

    w_t LT = Aut.getWeight(T.From, T.Sym, T.To);
    LocId FromLoc = toLocId(T.From);

    for (uint32_t RIdx : Sys.getRulesFor(FromLoc, T.Sym)) {
      const auto &Rule = Sys.getRule(RIdx);

      switch (Rule.Kind) {
      case WPDSRuleKind::Pop: {
        // (p, γ) ↪ (p', ε)  →  update (p', ε, q)
        tryUpdate({toStateId(Rule.ToLoc), kEpsilonSym, T.To},
                  LT.extend(Rule.Wt));
        break;
      }
      case WPDSRuleKind::Internal: {
        // (p, γ) ↪ (p', γ')  →  update (p', γ', q)
        tryUpdate({toStateId(Rule.ToLoc), Rule.ToSym1, T.To},
                  LT.extend(Rule.Wt));
        break;
      }
      case WPDSRuleKind::Push: {
        // (p, γ) ↪ (p', γ'γ'')
        StateId QW = Aut.getOrCreateWitnessState(Rule.ToLoc, Rule.ToSym1);

        // Call transition (p', γ', q_W) with weight 1 — always idempotent.
        tryUpdate({toStateId(Rule.ToLoc), Rule.ToSym1, QW}, w_t::one());

        // Return continuation (q_W, γ'', q) with weight l(T) ⊗ f(rule).
        w_t RetW = LT.extend(Rule.Wt);
        bool RetChanged = tryUpdate({QW, Rule.ToSym2, T.To}, RetW);

        if (RetChanged) {
          // Propagate through existing ε-transitions into q_W (Algorithm 3,
          // line 23). Weight: RetW ⊗ l(ε-trans) = RetW.extend(LEps).
          for (StateId Src : Aut.getEpsilonPredecessors(QW)) {
            w_t LEps = Aut.getWeight(Src, kEpsilonSym, QW);
            tryUpdate({Src, Rule.ToSym2, T.To}, RetW.extend(LEps));
          }
        }
        break;
      }
      }
    }
  }

  void processEpsilonTransition(const Transition &T) {
    // t = (p, ε, q): propagate through all transitions leaving q.
    w_t LEps = Aut.getWeight(T.From, kEpsilonSym, T.To);
    for (auto [Sym, To] : Aut.getOutgoing(T.To)) {
      w_t LOut = Aut.getWeight(T.To, Sym, To);
      tryUpdate({T.From, Sym, To}, LOut.extend(LEps));
    }
  }

  bool tryUpdate(Transition T, const w_t &V) {
    bool Changed = Aut.update(T.From, T.Sym, T.To, V);
    if (Changed) {
      Worklist.push_back(T);
    }
    return Changed;
  }

  // ─── Phase 4: Algorithm 4 — compute V_{d, γ_n} ─────────────────────────────

  void computeNodeValues() {
    const uint32_t NumStates = Aut.getNumStates();

    // W[q] = accumulated backwards value for state q. Initialised to zero().
    TypedVector<StateId, w_t> W(NumStates, w_t::zero());
    W[QFinal] = w_t::one();

    // Build backward adjacency: To → list of (From, (Sym, weight)).
    TypedVector<StateId,
                llvm::SmallVector<std::pair<StateId, std::pair<SymId, w_t>>, 4>>
        BackwardEdges(NumStates);
    for (const auto &[Trans, Wt] : Aut.getWeights()) {
      BackwardEdges[Trans.To].push_back({Trans.From, {Trans.Sym, Wt}});
    }

    // Backwards worklist: propagate from q_f towards initial states.
    std::deque<StateId> BWList;
    BWList.push_back(QFinal);
    BitSet<StateId> Enqueued;
    Enqueued.insert(QFinal);

    while (!BWList.empty()) {
      StateId Q = BWList.front();
      BWList.pop_front();
      Enqueued.erase(Q);

      // Copy by value: if BackwardEdges[Q] contains a self-loop (Src == Q),
      // the assignment W[Q] = move(NewWSrc) would leave a reference dangling.
      w_t WQ = W[Q];

      for (auto &[Src, SymAndWt] : BackwardEdges[Q]) {
        auto &[Sym, LTrans] = SymAndWt;
        w_t Contrib = LTrans.extend(WQ);
        w_t NewWSrc = W[Src].combine(Contrib);

        if (NewWSrc == W[Src]) {
          continue;
        }

        W[Src] = std::move(NewWSrc);
        if (!Enqueued.contains(Src)) {
          Enqueued.insert(Src);
          BWList.push_back(Src);
        }
      }
    }

    // Extract V_{d, γ_n}: for each non-ε transition from an initial state.
    for (const auto &[Trans, LTrans] : Aut.getWeights()) {
      if (Trans.Sym == kEpsilonSym) {
        continue;
      }
      if (!Aut.isInitial(Trans.From)) {
        continue;
      }
      // Trans.From encodes the LocId (factId); Trans.Sym is the SymId (node).
      auto Key = std::make_pair(toLocId(Trans.From), Trans.Sym);
      w_t Val = LTrans.extend(W[Trans.To]);

      auto [It, Inserted] = NodeValues.try_emplace(Key, Val);
      if (!Inserted) {
        It->second = It->second.combine(Val);
      }
    }
  }

  // ─── Data members ──────────────────────────────────────────────────────────

  ProblemTy &Problem;

  /// Compressor: dataflow facts d_t → LocId (control locations).
  Compressor<d_t, LocId> FactComp;
  /// Compressor: program nodes n_t → SymId (stack symbols).
  Compressor<n_t, SymId> NodeComp;

  /// The weighted pushdown system built from the ICFG.
  WeightedPushdownSystem<w_t> Sys;

  /// The P-automaton (initially A_0, then saturated to A_{post*}).
  AutomatonTy Aut;

  /// The single accepting state q_f.
  StateId QFinal{0};

  /// Worklist for Algorithm 3.
  std::deque<Transition> Worklist;

  /// Per-(LocId, SymId) meet-over-all-paths values V_{d, γ_n}.
  typename WPDSSolverResults<w_t>::NodeValueMap NodeValues;
};

} // namespace wpds
} // namespace psr
