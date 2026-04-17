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
#include "phasar/DataFlow/WPDS/WPDSProblem.h"
#include "phasar/DataFlow/WPDS/WPDSRule.h"
#include "phasar/DataFlow/WPDS/WPDSSolverResults.h"
#include "phasar/Utils/Compressor.h"

#include "llvm/ADT/DenseMap.h"
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
///   control locations = dataflow facts (d_t)
///   stack symbols     = program nodes  (n_t), compressed to uint32_t
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
  using StateId = typename AutomatonTy::StateId;
  using Transition = typename AutomatonTy::Transition;

public:
  explicit WPDSSolver(ProblemTy &Problem) : Problem(Problem) {}

  /// Run the full analysis: build WPDS, saturate, extract node values.
  void solve() {
    compressFacts();
    buildWPDS();
    buildInitialAutomaton();
    saturate();
    computeNodeValues();
  }

  /// Returns the saturated P-automaton and per-(fact,node) values.
  [[nodiscard]] WPDSSolverResults<w_t> getResults() && {
    return WPDSSolverResults<w_t>(std::move(Aut), std::move(NodeValues));
  }

  /// Returns the compressed ID of a dataflow fact (control location).
  [[nodiscard]] uint32_t factId(const d_t &D) const { return FactComp.get(D); }

  /// Returns the compressed ID of a program node (stack symbol).
  [[nodiscard]] uint32_t symId(n_t N) const { return NodeComp.get(N); }

private:
  // ─── Phase 0: Compress facts ───────────────────────────────────────────────

  void compressFacts() {
    for (const d_t &D : Problem.getAllFacts())
      FactComp.getOrInsert(D);
  }

  // ─── Phase 1: Build WPDS from ICFG ─────────────────────────────────────────

  void buildWPDS() {
    const i_t &ICFG = Problem.getICFG();
    const uint32_t NumFacts = static_cast<uint32_t>(FactComp.size());

    for (auto Fun : ICFG.getAllFunctions()) {
      for (auto N : ICFG.getAllInstructionsOf(Fun)) {
        uint32_t NSym = NodeComp.getOrInsert(N);

        if (ICFG.isCallSite(N)) {
          // Successors of a call site are the return sites.
          for (auto R : ICFG.getSuccsOf(N)) {
            uint32_t RSym = NodeComp.getOrInsert(R);

            for (auto Callee : ICFG.getCalleesOfCallAt(N)) {
              for (auto E : ICFG.getStartPointsOf(Callee)) {
                uint32_t ESym = NodeComp.getOrInsert(E);

                // Push rules: call edge n→e with continuation r
                for (uint32_t DIdx = 0; DIdx < NumFacts; ++DIdx) {
                  const d_t &D = FactComp[DIdx];
                  for (auto [Dprime, W] : Problem.callFlowWeights(N, E, R, D)) {
                    uint32_t DPrimeIdx = FactComp.getOrInsert(Dprime);
                    Sys.addPushRule(DIdx, NSym, DPrimeIdx, ESym, RSym,
                                    std::move(W));
                  }
                }
              }
            }

            // Internal rules: call-to-return bypass n→r
            for (uint32_t DIdx = 0; DIdx < NumFacts; ++DIdx) {
              const d_t &D = FactComp[DIdx];
              for (auto [Dprime, W] :
                   Problem.callToReturnFlowWeights(N, R, D)) {
                uint32_t DPrimeIdx = FactComp.getOrInsert(Dprime);
                Sys.addInternalRule(DIdx, NSym, DPrimeIdx, RSym, std::move(W));
              }
            }
          }

        } else if (ICFG.isExitInst(N)) {
          // Pop rules: return from exit node
          for (uint32_t DIdx = 0; DIdx < NumFacts; ++DIdx) {
            const d_t &D = FactComp[DIdx];
            for (auto [Dprime, W] : Problem.returnFlowWeights(N, D)) {
              uint32_t DPrimeIdx = FactComp.getOrInsert(Dprime);
              Sys.addPopRule(DIdx, NSym, DPrimeIdx, std::move(W));
            }
          }

        } else {
          // Internal rules: intraprocedural edge n→succ
          for (auto Succ : ICFG.getSuccsOf(N)) {
            uint32_t SuccSym = NodeComp.getOrInsert(Succ);
            for (uint32_t DIdx = 0; DIdx < NumFacts; ++DIdx) {
              const d_t &D = FactComp[DIdx];
              for (auto [Dprime, W] : Problem.intraFlowWeights(N, Succ, D)) {
                uint32_t DPrimeIdx = FactComp.getOrInsert(Dprime);
                Sys.addInternalRule(DIdx, NSym, DPrimeIdx, SuccSym,
                                    std::move(W));
              }
            }
          }
        }
      }
    }
  }

  // ─── Phase 2: Build initial P-automaton ────────────────────────────────────

  void buildInitialAutomaton() {
    const uint32_t NumFacts = static_cast<uint32_t>(FactComp.size());

    // Allocate one automaton state per control location (= fact).
    // State ID i corresponds to factId i.
    for (uint32_t I = 0; I < NumFacts; ++I) {
      StateId S = Aut.addState();
      assert(S == I && "State IDs must match fact IDs");
      Aut.markInitial(S);
    }

    // Single accepting state q_f (state ID = NumFacts).
    QFinal = Aut.addState();
    Aut.markFinal(QFinal);

    // Pre-create witness states for push rules (Phase I of Algorithm 3).
    for (const auto &Rule : Sys.getAllRules()) {
      if (Rule.Kind == WPDSRuleKind::Push)
        Aut.getOrCreateWitnessState(Rule.ToLoc, Rule.ToSym1);
    }

    // Initial transitions: (factId(zeroFact), γ_e, QFinal) for each entry e.
    uint32_t ZeroFactId = FactComp.get(Problem.getZeroFact());
    for (auto E : Problem.getEntryPoints()) {
      uint32_t ESym = NodeComp.getOrInsert(E);
      w_t InitW = Problem.getInitialWeight(E);
      bool Changed = Aut.update(ZeroFactId, ESym, QFinal, InitW);
      if (Changed)
        Worklist.push_back({ZeroFactId, ESym, QFinal});
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
    // l(T): the current weight of this transition.
    w_t LT = Aut.getWeight(T.From, T.Sym, T.To);

    for (uint32_t RIdx : Sys.getRulesFor(T.From, T.Sym)) {
      const auto &Rule = Sys.getRule(RIdx);

      switch (Rule.Kind) {
      case WPDSRuleKind::Pop: {
        // (p, γ) ↪ (p', ε)  →  update (p', ε, q)
        w_t NewW = LT.extend(Rule.Wt);
        tryUpdate({Rule.ToLoc, kEpsilonSym, T.To}, NewW);
        break;
      }
      case WPDSRuleKind::Internal: {
        // (p, γ) ↪ (p', γ')  →  update (p', γ', q)
        w_t NewW = LT.extend(Rule.Wt);
        tryUpdate({Rule.ToLoc, Rule.ToSym1, T.To}, NewW);
        break;
      }
      case WPDSRuleKind::Push: {
        // (p, γ) ↪ (p', γ'γ'')  →  call trans + return continuation
        StateId QW = Aut.getOrCreateWitnessState(Rule.ToLoc, Rule.ToSym1);

        // Call transition (p', γ', q_W) with weight 1 — idempotent.
        tryUpdate({Rule.ToLoc, Rule.ToSym1, QW}, w_t::one());

        // Return continuation (q_W, γ'', q) with weight l(T) ⊗ f(rule).
        w_t RetW = LT.extend(Rule.Wt);
        bool RetChanged = tryUpdate({QW, Rule.ToSym2, T.To}, RetW);

        if (RetChanged) {
          // Propagate through any existing ε-transitions into q_W.
          // These arise from pop rules applied to the call transition.
          // Per Algorithm 3 line 23: weight = l(t') ⊗ l(ε-trans)
          //   where t' = (q_W, γ'', q) with weight RetW
          //   and   l(ε-trans) = l(Src, ε, q_W) = LEps
          // Combined: RetW ⊗ LEps  →  RetW.extend(LEps)
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
    // t = (p, ε, q): for each outgoing transition t' = (q, γ', q') from q,
    // update (p, γ', q') with l(t') ⊗ l(t).
    w_t LEps = Aut.getWeight(T.From, kEpsilonSym, T.To);
    for (auto [Sym, To] : Aut.getOutgoing(T.To)) {
      w_t LOut = Aut.getWeight(T.To, Sym, To);
      tryUpdate({T.From, Sym, To}, LOut.extend(LEps));
    }
  }

  /// Combine V into the weight of transition T; if improved, add T to worklist.
  bool tryUpdate(Transition T, const w_t &V) {
    bool Changed = Aut.update(T.From, T.Sym, T.To, V);
    if (Changed)
      Worklist.push_back(T);
    return Changed;
  }

  // ─── Phase 4: Algorithm 4 — compute V_{d, γ_n} ─────────────────────────────

  /// Algorithm 4 (Fig. 19): backwards propagation through the saturated
  /// automaton.
  ///
  /// For each non-initial, non-final state q, compute:
  ///   W_q = ⊕_{paths p→*→q_f reading w, passing through q} product of weights
  ///
  /// In practice: propagate backwards from final states.
  ///   Initialize W_{q_f} = w_t::one().
  ///   For each transition (p, γ, q) in reverse, combine W_q into V_{p,γ}.
  ///
  /// Then V_{d, γ_n} = ⊕_{t=(factId(d), γ_n, q)} l(t) ⊗ W_q.
  void computeNodeValues() {
    const uint32_t NumStates = Aut.getNumStates();
    // W[q] = accumulated backwards value for state q.
    llvm::SmallVector<w_t> W(NumStates, w_t::zero());

    // Final state starts with one().
    W[QFinal] = w_t::one();

    // Backwards worklist: process states in reverse topological order.
    // For simplicity use a vector worklist; correctness holds for any order
    // (idempotent semiring ⊕ ensures fixpoint).
    std::deque<StateId> BWList;
    BWList.push_back(QFinal);
    llvm::SmallVector<bool> Enqueued(NumStates, false);
    Enqueued[QFinal] = true;

    // Build a backward index: state → list of incoming (from, sym, weight).
    // We iterate over all transitions in the automaton.
    llvm::SmallVector<
        llvm::SmallVector<std::pair<StateId, std::pair<uint32_t, w_t>>, 4>>
        BackwardEdges(NumStates);
    for (const auto &[Trans, Wt] : Aut.getWeights()) {
      BackwardEdges[Trans.To].push_back({Trans.From, {Trans.Sym, Wt}});
    }

    while (!BWList.empty()) {
      StateId Q = BWList.front();
      BWList.pop_front();
      Enqueued[Q] = false;

      const w_t &WQ = W[Q];

      for (auto &[Src, SymAndWt] : BackwardEdges[Q]) {
        auto &[Sym, LTrans] = SymAndWt;

        // W[Src] receives: l(trans) ⊗ W[Q]
        w_t Contribution = LTrans.extend(WQ);
        w_t NewWSrc = W[Src].combine(Contribution);

        if (NewWSrc == W[Src])
          continue;

        W[Src] = std::move(NewWSrc);
        if (!Enqueued[Src]) {
          Enqueued[Src] = true;
          BWList.push_back(Src);
        }
      }
    }

    // Extract V_{d, γ_n} = ⊕_{t=(factId(d), γ_n, q)} l(t) ⊗ W[q].
    // Transitions from initial states (factId states) with non-ε symbols give
    // the meet-over-all-paths values.
    for (const auto &[Trans, LTrans] : Aut.getWeights()) {
      if (Trans.Sym == kEpsilonSym)
        continue;
      if (!Aut.isInitial(Trans.From))
        continue;
      // Trans.From = factId, Trans.Sym = symId(node)
      auto Key = std::make_pair(Trans.From, Trans.Sym);
      w_t Val = LTrans.extend(W[Trans.To]);

      auto [It, Inserted] = NodeValues.try_emplace(Key, Val);
      if (!Inserted)
        It->second = It->second.combine(Val);
    }
  }

  // ─── Data members ──────────────────────────────────────────────────────────

  ProblemTy &Problem;

  /// Compressor for dataflow facts (d_t → uint32_t control location IDs).
  Compressor<d_t> FactComp;
  /// Compressor for program nodes (n_t → uint32_t stack symbol IDs).
  Compressor<n_t> NodeComp;

  /// The weighted pushdown system built from the ICFG.
  WeightedPushdownSystem<w_t> Sys;

  /// The P-automaton (initially A_0, then saturated to A_{post*}).
  AutomatonTy Aut;

  /// The single accepting state q_f.
  StateId QFinal = 0;

  /// Worklist for Algorithm 3.
  std::deque<Transition> Worklist;

  /// Per-(factId, symId) meet-over-all-paths values V_{d, γ_n}.
  typename WPDSSolverResults<w_t>::NodeValueMap NodeValues;
};

} // namespace wpds
} // namespace psr
