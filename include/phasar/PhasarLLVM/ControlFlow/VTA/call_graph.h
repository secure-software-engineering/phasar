/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_CALL_GRAPH_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_CALL_GRAPH_H

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "llvm/IR/Module.h"

namespace psr {

[[nodiscard]] psr::CallGraph<const llvm::Instruction *, const llvm::Function *>
computeVTACallgraph(const llvm::Module &Mod,
                    const psr::CallGraph<const llvm::Instruction *,
                                         const llvm::Function *> &BaseCG,
                    psr::LLVMAliasInfoRef AS,
                    const psr::LLVMVFTableProvider &VTP);

} // namespace psr

#endif
