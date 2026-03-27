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
#include "phasar/Utils/TypeTraits.h"

#include <concepts>

namespace psr::monoifds {

template <typename T>
concept MonoIFDSAnalysisDomain = IsAnalysisDomain<T>;

template <typename T>
concept MonoIFDSProblem =
    requires(T &Problem,
             DataFlowEnvironment<typename T::ProblemAnalysisDomain::d_t> &InOut,
             typename T::ProblemAnalysisDomain::n_t Inst,
             const typename T::ProblemAnalysisDomain::n_t &Fact,
             const typename T::ProblemAnalysisDomain::f_t &Fun) {
      typename T::ProblemAnalysisDomain;
      requires MonoIFDSAnalysisDomain<typename T::ProblemAnalysisDomain>;

      Problem.normalFlow(InOut, Inst);
      Problem.callToRetFlow(InOut, Inst);
      {
        Problem.returnFlow(Inst, Fact)
      } -> psr::is_iterable_over_v<typename T::ProblemAnalysisDomain::d_t>;

      {
        Problem.invReturnFlow(Inst, Fact)
      } -> psr::is_iterable_over_v<typename T::ProblemAnalysisDomain::d_t>;

      {
        Problem.getZeroValue()
      } -> std::convertible_to<typename T::ProblemAnalysisDomain::d_t>;

      Problem.initialSeeds(InOut, Fun);
    };
} // namespace psr::monoifds
