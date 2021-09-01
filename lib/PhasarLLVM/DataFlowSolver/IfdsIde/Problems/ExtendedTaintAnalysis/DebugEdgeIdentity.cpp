/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/DebugEdgeIdentity.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::XTaint {
DebugEdgeIdentity::DebugEdgeIdentity(const llvm::Instruction *Inst)
    : Inst(Inst) {}

EdgeDomain DebugEdgeIdentity::computeTarget(EdgeDomain source) {
  return source;
}

auto DebugEdgeIdentity::composeWith(EdgeFunctionPtrType secondFunction)
    -> EdgeFunctionPtrType {
  return secondFunction;
}

auto DebugEdgeIdentity::joinWith(EdgeFunctionPtrType otherFunction)
    -> EdgeFunctionPtrType {
  if ((otherFunction.get() == this) ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *ab = dynamic_cast<AllBottom<EdgeDomain> *>(otherFunction.get())) {
    return otherFunction;
  }
  if (auto *at = dynamic_cast<AllTop<EdgeDomain> *>(otherFunction.get())) {
    return this->shared_from_this();
  }

  if (dynamic_cast<DebugEdgeIdentity *>(&*otherFunction))
    return shared_from_this();
  // do not know how to join; hence ask other function to decide on this
  return otherFunction->joinWith(this->shared_from_this());
}

bool DebugEdgeIdentity::equal_to(EdgeFunctionPtrType other) const {
  if (this == &*other)
    return true;
  if (auto otherId = dynamic_cast<DebugEdgeIdentity *>(&*other)) {
    return Inst == otherId->Inst;
  }
  return false;
}

void DebugEdgeIdentity::print(std::ostream &OS, bool isForDebug) const {
  OS << "EdgeId[" << llvmIRToShortString(Inst) << "]";
}
} // namespace psr::XTaint