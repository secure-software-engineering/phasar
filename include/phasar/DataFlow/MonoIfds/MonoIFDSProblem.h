#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/IfdsIdeDomain.h"
#include "phasar/DataFlow/MonoIfds/DataFlowEnvironment.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/TypeTraits.h"

#include <concepts>
#include <memory_resource>

/// \file
/// Defines requirements for an analysis problem that can be solved by the
/// MonoIFDSSolver.
///
/// Since MonoIFDS solves analyses bottom-up, each CG-SCC is analyzed in
/// isolation. The solver will call Problem.localAnalysis(...) once per SCC and
/// use the returned LocalMonoIFDSProblem to drive the solving process.
/// Although the solver currently is only single-threaded, you can expect that
/// (also in a multi-threaded future) LocalMonoIFDSProblem instances are not
/// accessed by multiple threads at a time, while different instances may be
/// solved on different threads in parallel.

namespace psr::monoifds {

/// \brief Defines requirements for a MonoIFDS-compatible analysis domain.
template <typename T>
concept MonoIFDSAnalysisDomain = IfdsAnalysisDomain<T>;

/// \brief CG-SCC-local analysis
template <typename T, typename Dom>
concept LocalMonoIFDSProblem = requires(
    T &Problem, DataFlowEnvironment<typename Dom::d_t> &InOut,
    const DataFlowEnvironment<typename Dom::d_t> &In, typename Dom::n_t Inst,
    const typename Dom::n_t &Fact, const typename Dom::f_t &Fun,
    Compressor<typename Dom::d_t, SourceFactId> &SeedCompressor) {
  /// Intra-procedural data-flow. Input facts are passed-in as InOut;
  /// modifications are performed in-place.
  ///
  /// Corresponds to the $flow()$ function in the paper.
  Problem.normalFlow(InOut, Inst);

  /// Intra-procedural data-flow at call-sites. Input facts are passed-in as
  /// InOut; modifications are performed in-place. Kills facts that may be
  /// strongly updated by the callee. Don't use it to *generate* facts.
  ///
  /// Corresponds to the $callFlow()$ function in the paper.
  Problem.callToRetFlow(InOut, Inst);

  /// Inter-procedural data-flow at exit-statements; Maps callee-facts back
  /// to the return-site in the caller. As with normal IFDS, this function
  /// will be called for each incoming Fact, that should be mapped back;
  /// return-site facts are returned by this function.
  ///
  /// Corresponds to the $returnVal()$ function in the paper.
  {
    Problem.returnFlow(Inst, Fact)
  } -> psr::is_iterable_over_v<typename Dom::d_t>;

  /// Inter-procedural data-flow at entry-statements; Maps
  /// callee-source-facts back to the call-site in the caller. This function
  /// will be called for each source Fact; call-site facts are returned by
  /// this function.
  ///
  /// Corresponds to the $passArgs^{-1}()$ function in the paper.
  {
    Problem.invCallFlow(Inst, Fact)
  } -> psr::is_iterable_over_v<typename Dom::d_t>;

  /// Applies a pre-computed summary of Fun at Inst into InOut, if
  /// applicable.
  ///
  /// Useful for pre-known taint-propagators and declaration-only library
  /// functions.
  ///
  /// \returns True, iff a summary was applied. This will take precedence
  /// over a summary that the solver may have computed for Fun!
  { Problem.summaryFlow(In, InOut, Inst, Fun) } -> std::convertible_to<bool>;

  /// The special zero-value, aka. $\Lambda$. Always holds. Facts that are
  /// generated unconditionally, originate from zero.
  ///
  /// Note: The solver guarantees that the zero-value always has
  /// SourceFactId 0.
  { Problem.getZeroValue() } -> std::convertible_to<typename Dom::d_t>;

  /// Approximates the source-facts that should hold at the entry of Fun.
  /// Input the facts in the InOut map as
  /// `SeedState[Fact].insert(SeedCompressor.getOrInsert(Fact))`
  ///
  /// Note: This is assumed to be a (conservative) over-approximation!
  Problem.initialSeeds(InOut, SeedCompressor, Fun);

  /// At a call-site Inst calling Fun, invokes the given callback for each
  /// fact that should be generated from zero there.
  ///
  /// Useful for taint sources.
  Problem.generateFactsAtCall(Inst, Fun, DummyFn<const typename Dom::d_t &>{});

  /// At a non-call-site Inst, invokes the given callback for each
  /// fact that should be generated from zero there.
  ///
  /// Useful for taint sources.
  Problem.generateFacts(Inst, DummyFn<const typename Dom::d_t &>{});

  /// Invokes the given callback for each LeakFact for which the solver
  /// should later call onResult(Inst, LeakFact), if LeakFacts holds at
  /// Inst. Here, Inst is assumed to be a call-site that may call Fun.
  ///
  /// Useful for taint sinks.
  Problem.requestResultCallbackAtCallSite(Inst, Fun,
                                          DummyFn<const typename Dom::d_t &>{});

  /// Invokes the given callback for each LeakFact for which the solver
  /// should later call onResult(Inst, LeakFact), if LeakFacts holds at
  /// Inst.
  ///
  /// Useful for taint sinks.
  Problem.requestResultCallback(Inst, DummyFn<const typename Dom::d_t &>{});

  /// Notifies the problem that a previously requested leak-Fact now is
  /// known to hold at Inst.
  ///
  /// Useful for reporting taint leaks.
  Problem.onResult(Inst, Fact);
};

/// \brief Defines requirements for an analysis problem that can be solved by
/// the MonoIFDSSolver.
template <typename T>
concept MonoIFDSProblem =
    requires(T &Problem, SCCId<FunctionId> CurrSCC,
             std::pmr::memory_resource *MRes, llvm::raw_ostream &OS) {
      /// The analysis domain. Defines the type of data-flow facts, and the IR
      /// on which the analysis can be run.
      typename T::ProblemAnalysisDomain;
      requires MonoIFDSAnalysisDomain<typename T::ProblemAnalysisDomain>;

      /// Create a local analysis for the given SCC.
      /// Use the given std::memory_resource to allocate node-based containers,
      /// if you have any.
      {
        Problem.localAnalysis(CurrSCC, MRes)
      } -> LocalMonoIFDSProblem<typename T::ProblemAnalysisDomain>;

      /// Pretty-print the analysis results into the given llvm::raw_ostream.
      Problem.emitTextReport(OS);
    };

/// Optional requirement for a MonoIFDSProblem to better filter function
/// summaries.
template <typename T>
concept HasShouldBeInSummary =
    requires(T &Problem, typename T::ProblemAnalysisDomain::d_t ExitFact,
             typename T::ProblemAnalysisDomain::n_t ExitInst) {
      {
        Problem.shouldBeInSummary(ExitFact, ExitInst)
      } -> std::convertible_to<bool>;
    };
} // namespace psr::monoifds
