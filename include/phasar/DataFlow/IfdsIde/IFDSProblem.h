#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IfdsIdeDomain.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/Utils/TypeTraits.h"

#include <concepts>

namespace psr {
template <typename T>
concept IFDSProblem =
    FlowFunctionFactory<T> &&
    requires(T &P, const T &CP,
             typename T::ProblemAnalysisDomain::d_t FlowFact) {
      typename T::ProblemAnalysisDomain;
      requires IfdsAnalysisDomain<typename T::ProblemAnalysisDomain>;

      { CP.isZeroValue(FlowFact) } -> std::convertible_to<bool>;
      {
        CP.getZeroValue()
      } -> std::convertible_to<typename T::ProblemAnalysisDomain::d_t>;

      {
        P.initialSeeds()
      } -> std::convertible_to<
          InitialSeeds<typename T::ProblemAnalysisDomain::n_t,
                       typename T::ProblemAnalysisDomain::d_t,
                       typename detail::ValueDomainAdder<
                           typename T::ProblemAnalysisDomain>::l_t>>;

      { CP.getProjectIRDB() } -> ProjectIRDBConstPtr;
      { CP.getEntryPoints() } -> is_iterable_over_v<std::string>;
    };
} // namespace psr
