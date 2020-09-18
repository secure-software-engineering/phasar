// /******************************************************************************
//  * Copyright (c) 2018 Philipp Schubert.
//  * All rights reserved. This program and the accompanying materials are made
//  * available under the terms of LICENSE.txt.
//  *
//  * Contributors:
//  *     Philipp Schubert and others
//  *****************************************************************************/

// #include "phasar/PhasarLLVM/SPDS/Solver/SyncPDSSolver.h"
// #include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
// #include "phasar/PhasarLLVM/DataFlowSolver/WPDS/Solver/LLVMWPDSSolver.h"
// #include
// <phasar/PhasarLLVM/DataFlowSolver/WPDS/Problems/WPDSAliasCollector.h>
// #include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

// namespace psr {

// SyncPDSSolver::SyncPDSSolver(LLVMBasedICFG &I) : I(I) {}

// std::set<const llvm::Value *> SyncPDSSolver::getAliasesOf(const llvm::Value
// *V)
// {
//   WPDSAliasCollector AC(I, WPDSType::FWPDS, SearchDirection::FORWARD);
//   LLVMWPDSSolver<const llvm::Value *, BinaryDomain, LLVMBasedICFG&>
//   WpdsSolver(AC); return {V};
// }

// }
