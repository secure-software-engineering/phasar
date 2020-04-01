/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_STRONGUPDATESTORE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_STRONGUPDATESTORE_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include <functional>

#include "llvm/IR/Instructions.h"

namespace psr {

template <typename D> class StrongUpdateStore : public FlowFunction<D> {
protected:
  const llvm::StoreInst *Store;
  std::function<bool(D)> Predicate;

public:
  StrongUpdateStore(const llvm::StoreInst *S, std::function<bool(D)> P)
      : Store(S), Predicate(P) {}
  virtual ~StrongUpdateStore() = default;

  std::set<D> computeTargets(D source) override {
    if (source == Store->getPointerOperand()) {
      return {};
    } else if (Predicate(source)) {
      return {source, Store->getPointerOperand()};
    } else {
      return {source};
    }
  }
};

} // namespace psr

#endif
