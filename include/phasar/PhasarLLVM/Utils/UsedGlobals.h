#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/UsedGlobalsHolder.h"

namespace llvm {
class Module;
class GlobalVariable;
class Function;

} // namespace llvm

namespace psr {
UsedGlobalsHolder<const llvm::GlobalVariable *> computeUsedGlobals(
    const llvm::Module &Mod,
    const Compressor<const llvm::Function *, FunctionId> &Functions,
    const SCCHolder<FunctionId> &SCCs,
    const SCCDependencyGraph<FunctionId> &Callers);

// Same as above overload, but uses LLVMProjectIRDB. We cannot directly use
// LLVMProjectIRDB here, as it would create a circular dependency between
// phasar_llvm_utils and phasar_llvm_db
UsedGlobalsHolder<const llvm::GlobalVariable *> computeUsedGlobals(
    const ProjectIRDB auto &IRDB,
    const Compressor<const llvm::Function *, FunctionId> &Functions,
    const SCCHolder<FunctionId> &SCCs,
    const SCCDependencyGraph<FunctionId> &Callers) {
  return computeUsedGlobals(*IRDB.getModule(), Functions, SCCs, Callers);
}
} // namespace psr
