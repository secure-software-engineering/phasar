#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/IFDSProblem.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/SemiRing.h"

namespace psr {
template <typename T>
concept IDEProblem =
    IFDSProblem<T> &&         //
    EdgeFunctionFactory<T> && //
    IsJoinLattice<T> &&       //
    IsSemiRing<T> &&          //
    requires {
      requires IdeAnalysisDomain<typename T::ProblemAnalysisDomain>;
      requires std::same_as<typename T::l_t,
                            typename T::ProblemAnalysisDomain::l_t>;
      requires std::same_as<typename T::l_t, typename T::EdgeFunctionType::l_t>;
    };
} // namespace psr
