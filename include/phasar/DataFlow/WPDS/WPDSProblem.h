/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#pragma once

#include "phasar/DataFlow/WPDS/Semiring.h"
#include "phasar/Utils/TypeTraits.h"

#include <concepts>
#include <utility>

namespace psr {
namespace wpds {

/// Concept satisfied by any type that can serve as a WPDS analysis domain.
///
/// \tparam n_t  Program node type (e.g. const llvm::Instruction*).
/// \tparam d_t  Dataflow fact type — serves as the **control location** in the
///              WPDS exploded-supergraph encoding (Section 4.3 of the paper).
/// \tparam w_t  Semiring weight type satisfying BoundedIdempotentSemiring.
/// \tparam i_t  Interprocedural CFG type.
template <typename AnalysisDomainTy>
concept WPDSAnalysisDomain = requires {
  typename AnalysisDomainTy::n_t;
  typename AnalysisDomainTy::d_t;
  typename AnalysisDomainTy::w_t;
  typename AnalysisDomainTy::i_t;
} && BoundedIdempotentSemiring<typename AnalysisDomainTy::w_t>;

/// Concept for a WPDS-based interprocedural dataflow problem.
///
/// This follows the **Section 4.3 exploded-supergraph encoding** of the WPDS
/// paper (Reps, Schwoon, Jha, Melski, SciCP 58, 2005):
///
///   - Control locations  = dataflow facts (d_t).
///   - Stack symbols      = program nodes  (n_t), compressed to uint32_t IDs.
///
/// The WPDSSolver generates WPDS rules by calling the flow-weight functions
/// for every source fact d ∈ getAllFacts():
///
///   - Intraprocedural edge n→n', fact d:
///       for each (d', w) in intraFlowWeights(n, n', d):
///         Internal rule  (factId(d), γ_n) ↪ (factId(d'), γ_{n'})  weight w
///
///   - Call from n to callee entry e, return site r, fact d:
///       for each (d', w) in callFlowWeights(n, e, r, d):
///         Push rule      (factId(d), γ_n) ↪ (factId(d'), γ_e γ_r)  weight w
///       for each (d'', w) in callToReturnFlowWeights(n, r, d):
///         Internal rule  (factId(d), γ_n) ↪ (factId(d''), γ_r)     weight w
///
///   - Exit node x, fact d (return site is implicit in stack — context-sens.):
///       for each (d', w) in returnFlowWeights(x, d):
///         Pop rule       (factId(d), γ_x) ↪ (factId(d'), ε)         weight w
///
/// The initial P-automaton has one transition per entry point e:
///   (factId(getZeroFact()), γ_e, q_final)  weighted by getInitialWeight(e).
///
/// A type P satisfies WPDSProblem<Domain> if it exposes:
///   auto intraFlowWeights(n_t Src, n_t Dst, d_t SrcFact)
///       -> iterable<pair<d_t, w_t>>
///   auto callFlowWeights(n_t CallSite, n_t CalleeEntry, n_t ReturnSite,
///                        d_t SrcFact)
///       -> iterable<pair<d_t, w_t>>
///   auto callToReturnFlowWeights(n_t CallSite, n_t ReturnSite, d_t SrcFact)
///       -> iterable<pair<d_t, w_t>>
///   auto returnFlowWeights(n_t ExitNode, d_t SrcFact)
///       -> iterable<pair<d_t, w_t>>
///   auto getAllFacts()           -> iterable<d_t>
///   d_t  getZeroFact()
///   auto getEntryPoints()        -> iterable<n_t>
///   w_t  getInitialWeight(n_t)   [typically w_t::one()]
///   const i_t& getICFG() const
template <typename P, typename AnalysisDomainTy>
concept WPDSProblem =
    WPDSAnalysisDomain<AnalysisDomainTy> &&
    requires(P &Problem, const P &CProblem, typename AnalysisDomainTy::n_t N1,
             typename AnalysisDomainTy::n_t N2,
             typename AnalysisDomainTy::n_t N3,
             typename AnalysisDomainTy::d_t D) {
      /// Flow weights for intraprocedural edge Src→Dst from fact D.
      {
        Problem.intraFlowWeights(N1, N2, D)
      } -> psr::is_iterable_over_v<std::pair<typename AnalysisDomainTy::d_t,
                                             typename AnalysisDomainTy::w_t>>;

      /// Flow weights for call edge CallSite→CalleeEntry (return site N3)
      /// from fact D.
      {
        Problem.callFlowWeights(N1, N2, N3, D)
      } -> psr::is_iterable_over_v<std::pair<typename AnalysisDomainTy::d_t,
                                             typename AnalysisDomainTy::w_t>>;

      /// Flow weights for the call-to-return-site bypass CallSite→ReturnSite
      /// from fact D.
      {
        Problem.callToReturnFlowWeights(N1, N2, D)
      } -> psr::is_iterable_over_v<std::pair<typename AnalysisDomainTy::d_t,
                                             typename AnalysisDomainTy::w_t>>;

      /// Flow weights for return at exit node ExitNode from fact D.
      /// The return site is NOT a parameter — context sensitivity is provided
      /// by the WPDS stack mechanism.
      {
        Problem.returnFlowWeights(N1, D)
      } -> psr::is_iterable_over_v<std::pair<typename AnalysisDomainTy::d_t,
                                             typename AnalysisDomainTy::w_t>>;

      /// All dataflow facts in the analysis domain.
      {
        Problem.getAllFacts()
      } -> psr::is_iterable_over_v<typename AnalysisDomainTy::d_t>;

      /// The "zero fact" (λ-fact / seed fact) used as the initial control
      /// location in the P-automaton.
      {
        Problem.getZeroFact()
      } -> std::convertible_to<typename AnalysisDomainTy::d_t>;

      /// Entry nodes from which the analysis starts.
      {
        Problem.getEntryPoints()
      } -> psr::is_iterable_over_v<typename AnalysisDomainTy::n_t>;

      /// Initial weight on the automaton transition for entry node N1.
      {
        Problem.getInitialWeight(N1)
      } -> std::convertible_to<typename AnalysisDomainTy::w_t>;

      /// The ICFG used to build the WPDS rules.
      {
        CProblem.getICFG()
      } -> std::convertible_to<const typename AnalysisDomainTy::i_t &>;
    };

} // namespace wpds
} // namespace psr
