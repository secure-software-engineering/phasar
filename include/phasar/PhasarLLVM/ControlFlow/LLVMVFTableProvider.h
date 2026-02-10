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
#include "phasar/Utils/HashUtils.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/DebugInfoMetadata.h"

#include <cstdint>
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
  [[nodiscard]] const LLVMVFTable *getVFTableOrNull(const llvm::DIType *Type,
                                                    uint32_t Index = 0) const;

  [[nodiscard]] const llvm::SmallDenseSet<uint32_t> &
  getVTableIndexInHierarchy(const llvm::DIType *DerivedType,
                            const llvm::DIType *BaseType) const;

  /// Supercedes DIBasedTypeHierarchy::removeVTablePrefix
  [[nodiscard]] static llvm::StringRef
  removeVTablePrefix(llvm::StringRef GlobName) noexcept;

  /// Supercedes DIBasedTypeHierarchy::isVTable
  [[nodiscard]] static bool isVTable(llvm::StringRef MangledVarName);

  [[nodiscard]] const llvm::GlobalVariable *
  getVFTableGlobal(const llvm::DIType *Type) const;

  [[nodiscard]] const llvm::GlobalVariable *
  getVFTableGlobal(llvm::StringRef ClearTypeName) const;

private:
  llvm::StringMap<const llvm::GlobalVariable *> ClearNameTVMap;
  std::unordered_map<std::pair<const llvm::DIType *, uint32_t>, LLVMVFTable,
                     PairHash>
      TypeVFTMap;
  std::unordered_map<
      const llvm::DIType *,
      llvm::SmallDenseMap<const llvm::DIType *, llvm::SmallDenseSet<uint32_t>>>
      BasesOfVirt;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_LLVMVFTABLEPROVIDER_H
