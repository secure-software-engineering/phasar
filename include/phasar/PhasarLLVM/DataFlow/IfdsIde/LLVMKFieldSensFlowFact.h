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

namespace psr {

template <unsigned K = 3, unsigned OffsetLimit = 1024,
          typename d_t = const llvm::Value *>
class LLVMKFieldSensFlowFact : public KFieldSensFlowFact<d_t, K, OffsetLimit> {
private:
  LLVMKFieldSensFlowFact(const KFieldSensFlowFact<d_t, K, OffsetLimit> &Parent)
      : KFieldSensFlowFact<d_t, K, OffsetLimit>(Parent) {}

public:
  LLVMKFieldSensFlowFact() = default;
  LLVMKFieldSensFlowFact(const LLVMKFieldSensFlowFact &) = default;
  LLVMKFieldSensFlowFact(LLVMKFieldSensFlowFact &&) = default;
  LLVMKFieldSensFlowFact &operator=(const LLVMKFieldSensFlowFact &) = default;
  LLVMKFieldSensFlowFact &operator=(LLVMKFieldSensFlowFact &&) = default;
  static LLVMKFieldSensFlowFact getNonIndirectionValue(d_t Value){
    LLVMKFieldSensFlowFact Result;
    Result.BaseValue = Value;
    return Result;
  }
  // LLVMKFieldSensFlowFact(d_t BaseValue)
  //     : KFieldSensFlowFact<d_t, K, OffsetLimit>(BaseValue) {}

  bool operator==(const LLVMKFieldSensFlowFact &Other) const {
    return std::tie(this->BaseValue, this->AccessPath, this->FollowedByAny) ==
           std::tie(Other.BaseValue, Other.AccessPath, Other.FollowedByAny);
  }

  bool operator!=(const LLVMKFieldSensFlowFact &Other) const {
    return !(*this == (Other));
  }

  bool operator<(const LLVMKFieldSensFlowFact &Other) const {
    return std::tie(this->BaseValue, this->AccessPath, this->FollowedByAny) <
           std::tie(Other.BaseValue, Other.AccessPath, Other.FollowedByAny);
  }

  LLVMKFieldSensFlowFact getStored(const llvm::Value *BaseValue) {
    return LLVMKFieldSensFlowFact(
        KFieldSensFlowFact<d_t, K, OffsetLimit>::getStored(BaseValue));
  }

  std::optional<LLVMKFieldSensFlowFact> getLoaded(const llvm::LoadInst *Load,
                                                  int64_t FollowedOffset = 0) {
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
    // The lhs offset is calculating by subtracting the gep offset from the rhs,
    // thus we need the minus here.
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
