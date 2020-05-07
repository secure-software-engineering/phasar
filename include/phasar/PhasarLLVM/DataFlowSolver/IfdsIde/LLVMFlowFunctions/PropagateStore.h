/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_PROPAGATESTORE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_PROPAGATESTORE_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

#include "llvm/IR/Instructions.h"

namespace psr {

template <typename D> class PropagateStore : public FlowFunction<D> {
protected:
  const llvm::StoreInst *Store;

public:
  PropagateStore(const llvm::StoreInst *S) : Store(S) {}
  virtual ~PropagateStore() = default;

  std::set<D> computeTargets(D source) override {
    if (Store->getValueOperand() == source) {
      return {source, Store->getPointerOperand()};
    }
    return {source};
  }
};

} // namespace psr

#endif
