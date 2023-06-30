/******************************************************************************
 * Copyright (c) 2023 Martin Mory.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Martin Mory and others
 *****************************************************************************/
#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMKFIELDSENSFLOWFACT_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMKFIELDSENSFLOWFACT_H

#include "phasar/DataFlow/IfdsIde/KFieldSensFlowFact.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/APInt.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include <initializer_list>
#include <optional>
#include <utility>

namespace psr {

const llvm::AllocaInst *getAllocaInst(const llvm::GetElementPtrInst *Gep);

std::optional<int64_t> getConstantOffset(const llvm::GetElementPtrInst *Gep);

std::pair<const llvm::AllocaInst *, std::optional<int64_t>>
getAllocaInstAndConstantOffset(const llvm::GetElementPtrInst *Gep);

template <unsigned K = 3, unsigned OffsetLimit = 1024,
          typename d_t = const llvm::Value *>
class LLVMKFieldSensFlowFact : public KFieldSensFlowFact<d_t, K, OffsetLimit> {
private:
  LLVMKFieldSensFlowFact(const KFieldSensFlowFact<d_t, K, OffsetLimit> &Parent)
      : KFieldSensFlowFact<d_t, K, OffsetLimit>(Parent) {}

public:
  LLVMKFieldSensFlowFact() = default;
  ~LLVMKFieldSensFlowFact() = default;
  LLVMKFieldSensFlowFact(const LLVMKFieldSensFlowFact &) = default;
  LLVMKFieldSensFlowFact &operator=(const LLVMKFieldSensFlowFact &) = default;
  LLVMKFieldSensFlowFact(LLVMKFieldSensFlowFact &&) noexcept = default;
  LLVMKFieldSensFlowFact &
  operator=(LLVMKFieldSensFlowFact &&) noexcept = default;

  static LLVMKFieldSensFlowFact getNonIndirectionValue(d_t Value) {
    LLVMKFieldSensFlowFact Result;
    Result.BaseValue = Value;
    return Result;
  }

  static LLVMKFieldSensFlowFact getZeroOffsetDerefValue(d_t Value) {
    LLVMKFieldSensFlowFact Result;
    Result.BaseValue = Value;
    Result.AccessPath.push_back(0);
    return Result;
  }

  static LLVMKFieldSensFlowFact
  getCustomOffsetDerefValue(d_t Value,
                            std::initializer_list<unsigned> Offsets) {
    LLVMKFieldSensFlowFact Result;
    Result.BaseValue = Value;
    for (unsigned Offset : Offsets) {
      Result.AccessPath.push_back(Offset);
    }
    return Result;
  }

  bool operator==(const LLVMKFieldSensFlowFact &Other) const {
    return std::tie(this->BaseValue, this->AccessPath, this->FollowedByAny) ==
           std::tie(Other.BaseValue, Other.AccessPath, Other.FollowedByAny);
  }

  bool operator!=(const LLVMKFieldSensFlowFact &Other) const {
    return !(*this == Other);
  }

  bool operator<(const LLVMKFieldSensFlowFact &Other) const {
    return std::tie(this->BaseValue, this->AccessPath, this->FollowedByAny) <
           std::tie(Other.BaseValue, Other.AccessPath, Other.FollowedByAny);
  }

  [[nodiscard]] bool overwrittenByStore(const llvm::StoreInst *Store,
                                        const llvm::Value *BaseVal,
                                        std::optional<int64_t> FollowedOffset) {
    const auto &DL = Store->getModule()->getDataLayout();
    return this->BaseValue == BaseVal &&
           this->firstIndirectionMatches(
               DL.getTypeAllocSize(Store->getValueOperand()->getType()),
               FollowedOffset);
  }

  LLVMKFieldSensFlowFact getStored(const llvm::Value *BaseValue,
                                   int64_t FollowedOffset = 0) {
    return LLVMKFieldSensFlowFact(
        KFieldSensFlowFact<d_t, K, OffsetLimit>::getStored(BaseValue,
                                                           FollowedOffset));
  }

  std::optional<LLVMKFieldSensFlowFact>
  getLoaded(const llvm::LoadInst *Load,
            std::optional<int64_t> FollowedOffset = 0) {
    const auto &DL = Load->getModule()->getDataLayout();
    const auto LoadSize = DL.getTypeAllocSize(Load->getType());
    const auto &Parent = KFieldSensFlowFact<d_t, K, OffsetLimit>::getLoaded(
        Load, LoadSize, FollowedOffset);
    if (Parent) {
      return LLVMKFieldSensFlowFact(Parent.value());
    }
    return std::nullopt;
  }

  LLVMKFieldSensFlowFact getSameAP(const llvm::Value *GeneratedBaseVal) {
    auto Result = *this;
    Result.BaseValue = GeneratedBaseVal;
    return Result;
  }

  LLVMKFieldSensFlowFact getOverapproximated(const llvm::Value *BaseVal) {
    LLVMKFieldSensFlowFact Result;
    Result.BaseValue = BaseVal;
    Result.FollowedByAny = true;
    return Result;
  }

  LLVMKFieldSensFlowFact getWithOffset(const llvm::GetElementPtrInst *Gep) {
    if (!Gep->hasAllConstantIndices()) {
      return LLVMKFieldSensFlowFact(
          KFieldSensFlowFact<d_t, K, OffsetLimit>::getFirstOverapproximated());
    }
    const auto &DL = Gep->getModule()->getDataLayout();
    llvm::APInt AccumulatedOffset(DL.getPointerSize() * 8, 0, true);
    Gep->accumulateConstantOffset(DL, AccumulatedOffset);
    // The lhs offset is calculating by subtracting the gep offset from the
    // rhs, thus we need the minus here.
    return LLVMKFieldSensFlowFact(
        KFieldSensFlowFact<d_t, K, OffsetLimit>::getWithOffset(
            -AccumulatedOffset.getSExtValue()));
  }

  LLVMKFieldSensFlowFact getFirstOverapproximated();
  void print(llvm::raw_ostream &OS) const {
    KFieldSensFlowFact<d_t, K, OffsetLimit>::print(OS);
  }

  // bool operator==(const llvm::Value *V) const {};
  inline operator const llvm::Value *() { return this->BaseValue; }
};

template <typename d_t> class LLVMKFieldSensFlowFact<0, 0, d_t> {
private:
  d_t Fact;

public:
  constexpr LLVMKFieldSensFlowFact getStored() { return *this; }
  LLVMKFieldSensFlowFact getLoaded();
  LLVMKFieldSensFlowFact getWithOffset();
  LLVMKFieldSensFlowFact getFirstOverapproximated();
};

template <unsigned K, unsigned OffsetLimit, typename d_t>
bool factMatchesLLVMValue(
    const LLVMKFieldSensFlowFact<K, OffsetLimit, d_t> &Fact,
    const llvm::Value *Value) {
  return Fact.getBaseValue() == Value;
}

template <unsigned K, unsigned OffsetLimit, typename d_t>
inline llvm::raw_ostream &
operator<<(llvm::raw_ostream &OS,
           const LLVMKFieldSensFlowFact<K, OffsetLimit, d_t> &FlowFact) {
  FlowFact.print(OS);
  return OS;
}

} // namespace psr

// Compatibility with LLVM Casting
namespace llvm {
template <unsigned K, unsigned OffsetLimit, typename d_t>
struct simplify_type<psr::LLVMKFieldSensFlowFact<K, OffsetLimit, d_t>> {
  using SimpleType = const llvm::Value *;

  static SimpleType getSimplifiedValue(
      const psr::LLVMKFieldSensFlowFact<K, OffsetLimit, d_t> &FF) noexcept {
    return FF.getBaseValue();
  }
};
} // namespace llvm

// Implementations of STL traits.
namespace std {
template <unsigned K, unsigned OffsetLimit, typename d_t>
struct hash<psr::LLVMKFieldSensFlowFact<K, OffsetLimit, d_t>> {
  size_t operator()(
      const psr::LLVMKFieldSensFlowFact<K, OffsetLimit, d_t> &FlowFact) const {
    return std::hash<const llvm::Value *>()(FlowFact.getBaseValue());
  }
};
} // namespace std

#endif /* PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMKFIELDSENSFLOWFACT_H */
