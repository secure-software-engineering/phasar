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

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedVarCFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "z3++.h"

namespace psr {

// class LLVMBasedVarICFG : public LLVMBasedICFG,
//                          public detail::LLVMBasedVarCFGImpl {
// public:
//   LLVMBasedVarICFG(LLVMProjectIRDB *IRDB, CallGraphAnalysisType CGType,
//                    llvm::ArrayRef<std::string> EntryPoints = {},
//                    DIBasedTypeHierarchy *TH = nullptr,
//                    LLVMAliasInfoRef PT = nullptr,
//                    const stringstringmap_t *StaticBackwardRenaming =
//                    nullptr);
// };

} // namespace psr

#endif
