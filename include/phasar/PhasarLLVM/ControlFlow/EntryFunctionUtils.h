/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_ENTRYFUNCTIONUTILS_H
#define PHASAR_PHASARLLVM_UTILS_ENTRYFUNCTIONUTILS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Function.h"

#include <string>
#include <vector>

namespace psr {
class LLVMProjectIRDB;

[[nodiscard]] std::vector<const llvm::Function *>
getEntryFunctions(const LLVMProjectIRDB &IRDB,
                  llvm::ArrayRef<std::string> EntryPoints);

[[nodiscard]] std::vector<llvm::Function *>
getEntryFunctionsMut(LLVMProjectIRDB &IRDB,
                     llvm::ArrayRef<std::string> EntryPoints);
} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_ENTRYFUNCTIONUTILS_H
