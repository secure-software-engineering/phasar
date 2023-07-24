/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Pointer/PointsToInfo.h"

#include "phasar/Pointer/PointsToInfoBase.h"
#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"

#include <array>
#include <utility>

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

  [[nodiscard]] v_t
  asPointerOrNullImpl(ByConstRef<o_t> Pointer) const noexcept {
    return Pointer;
  }

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<o_t> /*Pointer*/,
                     ByConstRef<n_t> /*AtInstruction*/) const {
    static PointsToSetTy Empty{};
    return &Empty;
  }

  [[nodiscard]] std::vector<v_t>
  getInterestingPointersAtImpl(ByConstRef<n_t> /*AtInstruction*/) const {
    return {};
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

  [[nodiscard]] std::optional<v_t>
  asPointerOrNullImpl(ByConstRef<o_t> Pointer) const noexcept {
    if (llvm::all_of(Pointer.AccessPath, [](auto Offs) { return Offs == 0; })) {
      return Pointer.V;
    }
    return std::nullopt;
  }

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<o_t> /*Pointer*/,
                     ByConstRef<n_t> /*AtInstruction*/) const {
    static PointsToSetTy Empty{};
    return &Empty;
  }

  [[nodiscard]] PointsToSetPtrTy
  getPointsToSetImpl(ByConstRef<v_t> /*Pointer*/,
                     ByConstRef<n_t> /*AtInstruction*/) const {
    static PointsToSetTy Empty{};
    return &Empty;
  }

  [[nodiscard]] std::vector<v_t>
  getInterestingPointersAtImpl(ByConstRef<n_t> /*AtInstruction*/) const {
    return {};
  }
};

[[maybe_unused]] void testTypeErasure() {
  DummyFieldInsensitivePointsToAnalysis PTA1;
  PointsToInfoRef<PointsToTraits<DummyFieldInsensitivePointsToAnalysis>>
      TEPTA1 = &PTA1;

  DummyFieldSensitivePointsToAnalysis PTA2;
  PointsToInfoRef<PointsToTraits<DummyFieldSensitivePointsToAnalysis>> TEPTA2 =
      &PTA2;

  PointsToInfo<PointsToTraits<DummyFieldInsensitivePointsToAnalysis>> TEPTA3(
      std::in_place_type<DummyFieldInsensitivePointsToAnalysis>);

  PointsToInfo<PointsToTraits<DummyFieldSensitivePointsToAnalysis>> TEPTA4(
      std::in_place_type<DummyFieldSensitivePointsToAnalysis>);
}

template class PointsToInfoBase<DummyFieldSensitivePointsToAnalysis>;

template class PointsToInfoRef<
    PointsToTraits<DummyFieldInsensitivePointsToAnalysis>>;
template class PointsToInfoRef<
    PointsToTraits<DummyFieldSensitivePointsToAnalysis>>;

} // namespace psr
