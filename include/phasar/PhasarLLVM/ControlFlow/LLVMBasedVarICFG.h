/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDVARICFG_H_

#include <z3++.h>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/VarICFG.h"

namespace psr {

class LLVMBasedVarICFG
    : public virtual LLVMBasedICFG,
      public virtual VarICFG<const llvm::Instruction *, const llvm::Function *,
                             z3::expr>,
      public virtual LLVMBasedVarCFG {
public:
  LLVMBasedVarICFG(ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                   const std::set<std::string> &EntryPoints = {},
                   LLVMTypeHierarchy *TH = nullptr,
                   LLVMPointsToInfo *PT = nullptr,
                   const stringstringmap_t *StaticBackwardRenaming = nullptr);

  ~LLVMBasedVarICFG() override = default;
};

} // namespace psr

#endif
