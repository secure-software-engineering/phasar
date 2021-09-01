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
    : EdgeFunctionBase(Kind::Compose, BBO), F(F), G(G) {}

auto ComposeEdgeFunction::computeTarget(l_t Source) -> l_t {
  return G->computeTarget(F->computeTarget(Source));
}

bool ComposeEdgeFunction::equal_to(EdgeFunctionPtrType other) const {

  if (auto EFC = dynamic_cast<ComposeEdgeFunction *>(&*other)) {

    auto ret = (&*F == &*EFC->F || F->equal_to(EFC->F)) &&
               (&*G == &*EFC->G || G->equal_to(EFC->G));

    // print(std::cerr);
    // EFC->print(std::cerr << (ret ? "==" : " != "));
    // std::cerr << "\n";

    return ret;
  }

  // print(std::cerr << "Compare ");
  // other->print(std::cerr << " with ");
  // std::cerr << "\n";

  return false;
}

llvm::hash_code ComposeEdgeFunction::getHashCode() const {
  // forget about F in order to be fast ...
  return llvm::hash_combine(XTaint::getHashCode(G));
}

void ComposeEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "COMP[" << this << "| " << F->str() << " , " << G->str() << " ]";
}
} // namespace psr::XTaint