/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/IR/Instruction.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinConstEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinEdgeFunction.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

namespace psr::XTaint {
JoinConstEdgeFunction::JoinConstEdgeFunction(
    BasicBlockOrdering &BBO, EdgeFunctionPtrType OtherFn,
    const llvm::Instruction *OtherConst)
    : EdgeFunctionBase(EFKind::JoinConst, BBO), OtherFn(std::move(OtherFn)),
      OtherConst(OtherConst) {
  assert(OtherConst &&
         "Join with 'NotSanitized' is always 'NotSanitized' and should "
         "therefore not be modeled by a JoinConstEdgeFunction");
}

JoinConstEdgeFunction::l_t JoinConstEdgeFunction::computeTarget(l_t Source) {
  return OtherFn->computeTarget(Source).join(
      Source.getSanitizer() ? Source : l_t{OtherConst}, &BBO);
}

JoinConstEdgeFunction::EdgeFunctionPtrType
JoinConstEdgeFunction::joinWith(EdgeFunctionPtrType OtherFunction) {
  if (dynamic_cast<psr::AllBottom<l_t> *>(&*OtherFunction)) {
    return OtherFunction;
  }
  if (dynamic_cast<psr::AllTop<l_t> *>(&*OtherFunction)) {
    return shared_from_this();
  }
  if (auto *Gen = dynamic_cast<GenEdgeFunction *>(&*OtherFunction)) {
    if (Gen->getSanitizer() == nullptr) {
      return OtherFunction;
    }

    auto Res = EdgeDomain(OtherConst).join(Gen->getSanitizer(), &BBO);

    // we never return Top, Bottom or Sanitized from a join with two sanitizers

    if (Res.isNotSanitized()) {
      return getGenEdgeFunction(BBO);
    }

    return makeEF<JoinConstEdgeFunction>(BBO, OtherFn, Res.getSanitizer());
  }
  if (&*OtherFunction == &*getAllSanitized()) {
    return shared_from_this();
  }

  return JoinEdgeFunction::create(BBO, shared_from_this(), OtherFunction);
}

bool JoinConstEdgeFunction::equal_to(EdgeFunctionPtrType OtherFunction) const {
  if (auto *OtherJC = dynamic_cast<JoinConstEdgeFunction *>(&*OtherFunction)) {
    return OtherConst == OtherJC->OtherConst &&
           (&*OtherFn == &*OtherJC->OtherFn ||
            OtherFn->equal_to(OtherJC->OtherFn));
  }
  return false;
}

llvm::hash_code JoinConstEdgeFunction::getHashCode() const {
  return llvm::hash_combine(OtherConst, XTaint::getHashCode(OtherFn));
}

void JoinConstEdgeFunction::print(llvm::raw_ostream &OS,
                                  [[maybe_unused]] bool IsForDebug) const {
  OS << "JOINC[" << this << "| " << *OtherFn << " with const "
     << llvmIRToShortString(OtherConst) << " ]";
}

} // namespace psr::XTaint
