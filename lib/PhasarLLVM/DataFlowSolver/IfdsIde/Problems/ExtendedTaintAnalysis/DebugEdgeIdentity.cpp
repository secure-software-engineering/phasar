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

EdgeDomain DebugEdgeIdentity::computeTarget(EdgeDomain Source) {
  return Source;
}

auto DebugEdgeIdentity::composeWith(EdgeFunctionPtrType SecondFunction)
    -> EdgeFunctionPtrType {
  return SecondFunction;
}

auto DebugEdgeIdentity::joinWith(EdgeFunctionPtrType OtherFunction)
    -> EdgeFunctionPtrType {
  if ((OtherFunction.get() == this) ||
      OtherFunction->equal_to(shared_from_this())) {
    return shared_from_this();
  }
  if (dynamic_cast<AllBottom<EdgeDomain> *>(OtherFunction.get())) {
    return OtherFunction;
  }
  if (dynamic_cast<AllTop<EdgeDomain> *>(OtherFunction.get())) {
    return shared_from_this();
  }
  if (dynamic_cast<DebugEdgeIdentity *>(&*OtherFunction)) {
    return shared_from_this();
  }
  // do not know how to join; hence ask other function to decide on this
  return OtherFunction->joinWith(shared_from_this());
}

bool DebugEdgeIdentity::equal_to(EdgeFunctionPtrType Other) const {
  if (this == &*Other) {
    return true;
  }
  if (const auto *OtherId = dynamic_cast<DebugEdgeIdentity *>(&*Other)) {
    return Inst == OtherId->Inst;
  }
  return false;
}

void DebugEdgeIdentity::print(llvm::raw_ostream &OS,
                              [[maybe_unused]] bool IsForDebug) const {
  OS << "EdgeId[" << llvmIRToShortString(Inst) << "]";
}
} // namespace psr::XTaint
