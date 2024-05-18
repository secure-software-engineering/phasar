/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCALLGRAPH_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCALLGRAPH_H

#include "phasar/ControlFlow/CallGraph.h"

namespace llvm {
class Instruction;
class Function;
} // namespace llvm

namespace psr {
using LLVMBasedCallGraph =
    CallGraph<const llvm::Instruction *, const llvm::Function *>;
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCALLGRAPH_H
