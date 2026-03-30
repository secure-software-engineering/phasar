#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/MonoIfds/DataFlowEnvironment.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/TypeTraits.h"

#include <concepts>
#include <memory_resource>

namespace psr::monoifds {

template <typename T>
concept MonoIFDSAnalysisDomain = IsAnalysisDomain<T>;

template <typename T, typename Dom>
concept LocalMonoIFDSProblem =
    requires(T &Problem, DataFlowEnvironment<typename Dom::d_t> &InOut,
             typename Dom::n_t Inst, const typename Dom::n_t &Fact,
             const typename Dom::f_t &Fun,
             Compressor<typename Dom::d_t, SourceFactId> &SeedCompressor) {
      Problem.normalFlow(InOut, Inst);
      Problem.callToRetFlow(InOut, Inst);
      {
        Problem.returnFlow(Inst, Fact)
      } -> psr::is_iterable_over_v<typename Dom::d_t>;

      {
        Problem.invReturnFlow(Inst, Fact)
      } -> psr::is_iterable_over_v<typename Dom::d_t>;

      { Problem.getZeroValue() } -> std::convertible_to<typename Dom::d_t>;

      Problem.initialSeeds(InOut, SeedCompressor, Fun);

      Problem.generateFactsAtCall(Inst, Fun,
                                  [](const typename Dom::d_t & GenFact) {});

      Problem.generateFacts(Inst, [](const typename Dom::d_t & GenFact) {});
      Problem.requestedEffectAtCall(Inst, Fun,
                                    [](const typename Dom::d_t & LeakFact) {});
      Problem.requestedEffect(Inst, [](const typename Dom::d_t & LeakFact) {});
      Problem.onResult(Inst, Fact);
    };

template <typename T>
concept MonoIFDSProblem =
    requires(T &Problem, SCCId<FunctionId> CurrSCC,
             std::pmr::memory_resource *MRes, llvm::raw_ostream &OS) {
      typename T::ProblemAnalysisDomain;
      requires MonoIFDSAnalysisDomain<typename T::ProblemAnalysisDomain>;

      {
        Problem.localAnalysis(CurrSCC, MRes)
      } -> LocalMonoIFDSProblem<typename T::ProblemAnalysisDomain>;
      Problem.emitTextReport(OS);
    };
} // namespace psr::monoifds
