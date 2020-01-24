/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARIATIONALICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARIATIONALICFG_H_

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>
#include <z3++.h>

namespace psr {

class LLVMBasedVariationalICFG
    : public virtual LLVMBasedICFG,
      public virtual VariationalICFG<const llvm::Instruction *,
                                     const llvm::Function *, z3::expr>,
      public virtual LLVMBasedVariationalCFG {
public:
  LLVMBasedVariationalICFG(ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                           const std::set<std::string> &EntryPoints = {},
                           LLVMTypeHierarchy *TH = nullptr,
                           LLVMPointsToInfo *PT = nullptr);
};

} // namespace psr

#endif
