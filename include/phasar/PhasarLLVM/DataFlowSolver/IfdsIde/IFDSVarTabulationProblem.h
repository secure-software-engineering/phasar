/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSVARTABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSVARTABULATIONPROBLEM_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVarTabulationProblem.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

namespace psr {

// template <typename N, typename D, typename F, typename T, typename V,
//           typename I>
// class IFDSVarTabulationProblem
//     : IDEVarTabulationProblem<N, D, F, T, V, BinaryDomain, I> {
// public:
//   IFDSVarTabulationProblem(IFDSTabulationProblem<N, D, F, T, V, I>
//   &IFDSProblem,
//                            LLVMBasedVarICFG &VarICF)
//       : IDEVarTabulationProblem<N, D, F, T, V, BinaryDomain, I>(IFDSProblem,
//                                                                 VarICF) {}
// };

} // namespace psr

#endif
