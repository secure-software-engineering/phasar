/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_WPDSGENKILLPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_WPDSGENKILLPROBLEM_H

#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSOptions.h"

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class WPDSGenKillProblem {
public:
  WPDSGenKillProblem() = default;
  virtual ~WPDSGenKillProblem() = default;
};

} // namespace psr

#endif
