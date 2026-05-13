#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/UsedGlobalsHolder.h"

namespace psr {
UsedGlobalsHolder<const llvm::GlobalVariable *> computeUsedGlobals(
    const LLVMProjectIRDB &IRDB,
    const Compressor<const llvm::Function *, FunctionId> &Functions,
    const SCCHolder<FunctionId> &SCCs,
    const SCCDependencyGraph<FunctionId> &Callers);
} // namespace psr
