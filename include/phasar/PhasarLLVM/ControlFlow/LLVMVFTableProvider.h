/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMVFTABLEPROVIDER_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMVFTABLEPROVIDER_H

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"

#include <unordered_map>

namespace llvm {
class Module;
class DIType;
class GlobalVariable;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;

class LLVMVFTableProvider {
public:
  explicit LLVMVFTableProvider(const llvm::Module &Mod);
  explicit LLVMVFTableProvider(const LLVMProjectIRDB &IRDB);

  [[nodiscard]] bool hasVFTable(const llvm::DIType *Type) const;
  [[nodiscard]] const LLVMVFTable *
  getVFTableOrNull(const llvm::DIType *Type) const;

private:
  std::unordered_map<const llvm::DIType *, LLVMVFTable> TypeVFTMap;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_LLVMVFTABLEPROVIDER_H
