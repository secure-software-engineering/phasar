// /******************************************************************************
//  * Copyright (c) 2018 Philipp Schubert.
//  * All rights reserved. This program and the accompanying materials are made
//  * available under the terms of LICENSE.txt.
//  *
//  * Contributors:
//  *     Philipp Schubert and others
//  *****************************************************************************/

// #include <phasar/PhasarLLVM/SPDS/Solver/SPDSSolver.h>
// #include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
// #include <phasar/PhasarLLVM/WPDS/Solver/LLVMWPDSSolver.h>
// #include <phasar/PhasarLLVM/WPDS/Problems/WPDSAliasCollector.h>
// #include <phasar/PhasarLLVM/Utils/BinaryDomain.h>

// namespace psr {

// SPDSSolver::SPDSSolver(LLVMBasedICFG &I) : I(I) {}

// std::set<const llvm::Value *> SPDSSolver::getAliasesOf(const llvm::Value *V)
// {
//   WPDSAliasCollector AC(I, WPDSType::FWPDS, SearchDirection::FORWARD);
//   LLVMWPDSSolver<const llvm::Value *, BinaryDomain, LLVMBasedICFG&>
//   WpdsSolver(AC); return {V};
// }

// }
