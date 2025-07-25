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

#include "llvm/ADT/StringMap.h"

#include <unordered_map>

namespace llvm {
class Module;
class DIType;
class GlobalVariable;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;

/// \brief A class that provides access to all C++ virtual function tables
/// (VTables) found in the target program.
///
/// Useful for constructing a call graph for a C++-based target.
/// \note This class only works, if the target program's IR was generated with
/// debug information. Pass `-g` to the compiler to achieve this.
class LLVMVFTableProvider {
public:
  explicit LLVMVFTableProvider(const llvm::Module &Mod);
  explicit LLVMVFTableProvider(const LLVMProjectIRDB &IRDB);

  [[nodiscard]] bool hasVFTable(const llvm::DIType *Type) const;
  [[nodiscard]] const LLVMVFTable *
  getVFTableOrNull(const llvm::DIType *Type) const;

  [[nodiscard]] const llvm::GlobalVariable *
  getVFTableGlobal(const llvm::DIType *Type) const;

  [[nodiscard]] const llvm::GlobalVariable *
  getVFTableGlobal(llvm::StringRef ClearTypeName) const;

private:
  llvm::StringMap<const llvm::GlobalVariable *> ClearNameTVMap;
  std::unordered_map<const llvm::DIType *, LLVMVFTable> TypeVFTMap;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_LLVMVFTABLEPROVIDER_H
