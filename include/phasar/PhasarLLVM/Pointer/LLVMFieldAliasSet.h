/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMFIELDALIASSET_H
#define PHASAR_PHASARLLVM_POINTER_LLVMFIELDALIASSET_H

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Pointer/AliasAnalysisType.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"

#include <cstddef>
#include <functional>
#include <memory>

namespace psr {

class LLVMBasePointerAliasSet {
public:
  using v_t = typename LLVMAliasInfoRef::v_t;
  using n_t = typename LLVMAliasInfoRef::n_t;
  using AliasSetTy = LLVMAliasInfoRef::AliasSetTy;
  using AliasSetPtrTy = std::unique_ptr<AliasSetTy>;

  constexpr LLVMBasePointerAliasSet(LLVMAliasInfoRef AS) noexcept : AS(AS) {}

  [[nodiscard]] bool isInterProcedural() const noexcept {
    return AS.isInterProcedural();
  }

  [[nodiscard]] AliasAnalysisType getAliasAnalysisType() const noexcept {
    return AS.getAliasAnalysisType();
  }

  [[nodiscard]] static const llvm::Value *
  getBasePointer(const llvm::Value *Pointer);

  [[nodiscard]] AliasResult alias(v_t Pointer1, v_t Pointer2,
                                  n_t AtInstruction = {}) const {
    return AS.alias(Pointer1, Pointer2, AtInstruction);
  }

  [[nodiscard]] AliasSetPtrTy getAliasSet(v_t Pointer,
                                          n_t AtInstruction = {}) const;

private:
  LLVMAliasInfoRef AS;
};

} // namespace psr
#if 0
namespace psr{
class LLVMFieldAliasSet {
public:
  struct AccessPath {
    const llvm::Value *BasePtr{};
    llvm::SmallVector<ptrdiff_t, 5> FieldAccesses;

    static constexpr ptrdiff_t TopOffset = PTRDIFF_MIN;

    bool operator==(const AccessPath &Other) const noexcept {
      return BasePtr == Other.BasePtr && FieldAccesses == Other.FieldAccesses;
    }
    bool operator!=(const AccessPath &Other) const noexcept {
      return !(*this == Other);
    }
  };

  using v_t = typename LLVMAliasInfoRef::v_t;
  using n_t = typename LLVMAliasInfoRef::n_t;
  using AliasSetTy = llvm::DenseSet<AccessPath>;
  using AliasSetPtrTy = std::unique_ptr<AliasSetTy>;

  explicit LLVMFieldAliasSet(
      LLVMAliasInfoRef AS,
      std::reference_wrapper<const llvm::DataLayout> DL) noexcept
      : AS(AS), DL(&DL.get()) {}

  [[nodiscard]] bool isInterProcedural() const noexcept {
    return AS.isInterProcedural();
  }

  [[nodiscard]] AliasAnalysisType getAliasAnalysisType() const noexcept {
    return AS.getAliasAnalysisType();
  }

  [[nodiscard]] AccessPath getAccessPath(const llvm::Value *Pointer) const;

  [[nodiscard]] AliasResult alias(v_t Pointer1, v_t Pointer2,
                                  n_t AtInstruction = {}) const {
    return AS.alias(Pointer1, Pointer2, AtInstruction);
  }

  [[nodiscard]] AliasSetPtrTy getAliasSet(v_t Pointer,
                                          n_t AtInstruction = {}) const;

private:
  LLVMAliasInfoRef AS;
  const llvm::DataLayout *DL{};
};
} // namespace psr

namespace llvm {
template <> struct DenseMapInfo<psr::LLVMFieldAliasSet::AccessPath> {
  using AccessPath = psr::LLVMFieldAliasSet::AccessPath;
  static AccessPath getEmptyKey() {
    return AccessPath{DenseMapInfo<const Value *>::getEmptyKey(), {}};
  }
  static AccessPath getTombstoneKey() {
    return AccessPath{DenseMapInfo<const Value *>::getTombstoneKey(), {}};
  }
  static auto getHashValue(const AccessPath &AP) {
    auto HC = hash_value(AP.BasePtr);
    auto HC2 =
        hash_combine_range(AP.FieldAccesses.begin(), AP.FieldAccesses.end());
    return hash_combine(HC, HC2);
  }
  static bool isEqual(const AccessPath &L, const AccessPath &R) noexcept {
    return L == R;
  }
};
} // namespace llvm

#endif

#endif // PHASAR_PHASARLLVM_POINTER_LLVMFIELDALIASSET_H
