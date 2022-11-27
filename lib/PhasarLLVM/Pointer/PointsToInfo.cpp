/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/PointsToInfo.h"

#include "phasar/PhasarLLVM/Pointer/PointsToInfoBase.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"

#include <array>

namespace llvm {
class Value;
class Instruction;
} // namespace llvm

namespace psr {
// Provide some dummy implementations here to test whether the CRTP interface is
// correctly instantiated

class DummyFieldInsensitivePointsToAnalysis;

template <> struct PointsToTraits<DummyFieldInsensitivePointsToAnalysis> {
  using v_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using o_t = const llvm::Value *;
  using PointsToSetTy = llvm::DenseSet<o_t>;
  using PointsToSetPtrTy = const PointsToSetTy *;
};

class DummyFieldInsensitivePointsToAnalysis
    : public PointsToInfoBase<DummyFieldInsensitivePointsToAnalysis> {
  friend PointsToInfoBase;

  [[nodiscard]] o_t
  asAbstractObjectImpl(ByConstRef<v_t> Pointer) const noexcept {
    return Pointer;
  }

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<o_t> /*Pointer*/,
                     ByConstRef<n_t> /*AtInstruction*/) const {
    static PointsToSetTy Empty{};
    return &Empty;
  }
};

template class PointsToInfoBase<DummyFieldInsensitivePointsToAnalysis>;

class DummyFieldSensitivePointsToAnalysis;
struct DummyFieldSensitiveAbstractObject {
  const llvm::Value *V;
  std::array<ptrdiff_t, 3> AccessPath;
};
struct DFSAODenseMapInfo {
  using o_t = DummyFieldSensitiveAbstractObject;
  static o_t getEmptyKey() noexcept {
    return {llvm::DenseMapInfo<const llvm::Value *>::getEmptyKey(), {}};
  }
  static o_t getTombstoneKey() noexcept {
    return {llvm::DenseMapInfo<const llvm::Value *>::getTombstoneKey(), {}};
  }
  static auto getHashValue(ByConstRef<o_t> Obj) noexcept {
    return llvm::hash_combine(Obj.V, Obj.AccessPath[0], Obj.AccessPath[1],
                              Obj.AccessPath[2]);
  }
  static bool isEqual(ByConstRef<o_t> LHS, ByConstRef<o_t> RHS) noexcept {
    return LHS.V == RHS.V && LHS.AccessPath == RHS.AccessPath;
  }
};

template <> struct PointsToTraits<DummyFieldSensitivePointsToAnalysis> {
  using v_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using o_t = DummyFieldSensitiveAbstractObject;
  using PointsToSetTy = llvm::DenseSet<o_t, DFSAODenseMapInfo>;
  using PointsToSetPtrTy = const PointsToSetTy *;
};

class DummyFieldSensitivePointsToAnalysis
    : public PointsToInfoBase<DummyFieldSensitivePointsToAnalysis> {
  friend PointsToInfoBase;

  [[nodiscard]] o_t
  asAbstractObjectImpl(ByConstRef<v_t> Pointer) const noexcept {
    return {Pointer, {0}};
  }

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<o_t> /*Pointer*/,
                     ByConstRef<n_t> /*AtInstruction*/) const {
    static PointsToSetTy Empty{};
    return &Empty;
  }
};

template class PointsToInfoBase<DummyFieldSensitivePointsToAnalysis>;

template class PointsToInfoRef<
    PointsToTraits<DummyFieldInsensitivePointsToAnalysis>>;
template class PointsToInfoRef<
    PointsToTraits<DummyFieldSensitivePointsToAnalysis>>;

} // namespace psr
