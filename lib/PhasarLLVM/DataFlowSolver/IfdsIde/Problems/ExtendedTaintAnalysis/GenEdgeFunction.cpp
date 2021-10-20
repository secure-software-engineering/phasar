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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/KillIfSanitizedEdgeFunction.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::XTaint {

GenEdgeFunction::GenEdgeFunction(BasicBlockOrdering &BBO,
                                 const llvm::Instruction *Sani)
    : EdgeFunctionBase(Kind::Gen, BBO), Sani(Sani) {}

GenEdgeFunction::l_t
GenEdgeFunction::computeTarget([[maybe_unused]] l_t Source) {
  return Sani;
}

GenEdgeFunction::EdgeFunctionPtrType
GenEdgeFunction::composeWith(EdgeFunctionPtrType SecondFunction) {
  if (isEdgeIdentity(&*SecondFunction)) {
    return shared_from_this();
  }

  if (dynamic_cast<AllBottom<l_t> *>(&*SecondFunction)) {
    return shared_from_this();
  }

  if (dynamic_cast<GenEdgeFunction *>(&*SecondFunction)) {
    return SecondFunction;
  }

  auto Res = SecondFunction->computeTarget(Sani);

  switch (Res.getKind()) {
  case EdgeDomain::Bot:
    // std::cerr << "Generate Bot by compose" << std::endl;
    return getAllBot();
  case EdgeDomain::Top:
    return getAllTop();
  case EdgeDomain::Sanitized:
    return getAllSanitized();
  default:
    return makeEF<GenEdgeFunction>(BBO, Res.getSanitizer());
  }
}

GenEdgeFunction::EdgeFunctionPtrType
GenEdgeFunction::joinWith(EdgeFunctionPtrType OtherFunction) {
  if (dynamic_cast<psr::AllBottom<l_t> *>(&*OtherFunction)) {
    return shared_from_this();
  }
  if (dynamic_cast<psr::AllTop<l_t> *>(&*OtherFunction)) {
    return OtherFunction;
  }
  if (&*getAllSanitized() == &*OtherFunction) {
    return shared_from_this();
  }

  if (Sani == nullptr) {
    return OtherFunction;
  }

  if (auto *Other = dynamic_cast<EdgeFunctionBase *>(&*OtherFunction)) {

    if (auto *OtherGen = llvm::dyn_cast<GenEdgeFunction>(Other)) {

      auto JoinSani = EdgeDomain(Sani).join(OtherGen->Sani, &BBO);
      switch (JoinSani.getKind()) {
      case EdgeDomain::Bot:
        std::cerr << "Generate Bot by join" << std::endl;
        return getAllBot();
      case EdgeDomain::Top:
        return getAllTop();
      case EdgeDomain::Sanitized:
        return getAllSanitized();
      default:
        return makeEF<GenEdgeFunction>(BBO, JoinSani.getSanitizer());
      }
    }

    if (auto *OtherJoin = llvm::dyn_cast<JoinConstEdgeFunction>(Other)) {

      auto Res = EdgeDomain(OtherJoin->getConstant()).join(Sani, &BBO);

      // we never return Top, Bottom or Sanitized from a join with two
      // sanitizers

      if (Res.isNotSanitized()) {
        return makeEF<GenEdgeFunction>(BBO, nullptr);
      }

      return makeEF<JoinConstEdgeFunction>(BBO, OtherJoin->getFunction(),
                                           Res.getSanitizer());
    }
  }

  if (isEdgeIdentity(&*OtherFunction)) {
    return getAllBot();
  }

  return makeEF<JoinConstEdgeFunction>(BBO, OtherFunction, Sani);
}

llvm::hash_code GenEdgeFunction::getHashCode() const {
  return llvm::hash_value(Sani);
}

bool GenEdgeFunction::equal_to(EdgeFunctionPtrType OtherFunction) const {
  if (auto *OtherGen = dynamic_cast<GenEdgeFunction *>(&*OtherFunction)) {
    return Sani == OtherGen->Sani;
  }
  return false;
}

void GenEdgeFunction::print(std::ostream &OS,
                            [[maybe_unused]] bool IsForDebug) const {
  OS << "GenEF[" << (Sani ? llvmIRToString(Sani) : "null") << "]";
}

} // namespace psr::XTaint