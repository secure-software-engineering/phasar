/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_PROPAGATELOAD_H_
#define PHASAR_PHASARLLVM_IFDSIDE_LLVMFLOWFUNCTIONS_PROPAGATELOAD_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

#include "llvm/IR/Instructions.h"

namespace psr {

template <typename D> class PropagateLoad : public FlowFunction<D> {
protected:
  const llvm::LoadInst *Load;

public:
  PropagateLoad(const llvm::LoadInst *L) : Load(L) {}
  virtual ~PropagateLoad() = default;

  std::set<D> computeTargets(D source) override {
    if (source == Load->getPointerOperand()) {
      return {source, Load};
    }
    return {source};
  }
};

} // namespace psr

#endif
