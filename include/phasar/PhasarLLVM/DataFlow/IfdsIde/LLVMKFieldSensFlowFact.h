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
  LLVMKFieldSensFlowFact getStored() {
    return LLVMKFieldSensFlowFact(
        KFieldSensFlowFact<d_t, K, OffsetLimit>::getStored());
  }

  std::optional<LLVMKFieldSensFlowFact> getLoaded(const llvm::LoadInst *Load,
                                                  int64_t FollowedOffset = 0) {
    const auto &DL = Load->getModule()->getDataLayout();
    const auto LoadSize = DL.getTypeAllocSize(Load->getType());
    const auto &Parent = KFieldSensFlowFact<d_t, K, OffsetLimit>::getLoaded(
        LoadSize, FollowedOffset);
    if (Parent) {
      return LLVMKFieldSensFlowFact(Parent.value());
    }
    return std::nullopt;
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
  void print(llvm::raw_ostream &OS) {
    KFieldSensFlowFact<d_t, K, OffsetLimit>::print(OS);
  }
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

#endif /* PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMKFIELDSENSFLOWFACT_H */
