#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionId.h"

namespace psr {
Compressor<const llvm::Function *, FunctionId>
compressFunctions(const LLVMBasedCallGraph &CG,
                  llvm::ArrayRef<const llvm::Function *> EntryPoints);
} // namespace psr
