/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinConstEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinEdgeFunction.h"

namespace psr::XTaint {

ComposeEdgeFunction::ComposeEdgeFunction(BasicBlockOrdering &BBO,
                                         EdgeFunctionPtrType F,
                                         EdgeFunctionPtrType G)
    : EdgeFunctionBase(EFKind::Compose, BBO), F(std::move(F)), G(std::move(G)) {
}

auto ComposeEdgeFunction::computeTarget(l_t Source) -> l_t {
  return G->computeTarget(F->computeTarget(Source));
}

bool ComposeEdgeFunction::equal_to(EdgeFunctionPtrType Other) const {

  if (auto *EFC = dynamic_cast<ComposeEdgeFunction *>(&*Other)) {
    return (&*F == &*EFC->F || F->equal_to(EFC->F)) &&
           (&*G == &*EFC->G || G->equal_to(EFC->G));
  }

  return false;
}

llvm::hash_code ComposeEdgeFunction::getHashCode() const {
  // forget about F in order to be fast ...
  return llvm::hash_combine(XTaint::getHashCode(G));
}

void ComposeEdgeFunction::print(llvm::raw_ostream &OS,
                                [[maybe_unused]] bool IsForDebug) const {
  OS << "COMP[" << this << "| " << F->str() << " , " << G->str() << " ]";
}
} // namespace psr::XTaint
