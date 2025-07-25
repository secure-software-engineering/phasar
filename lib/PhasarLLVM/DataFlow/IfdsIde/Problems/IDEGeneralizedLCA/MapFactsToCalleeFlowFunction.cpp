/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCalleeFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

namespace psr::glca {

MapFactsToCalleeFlowFunction::MapFactsToCalleeFlowFunction(
    const llvm::CallBase *CallSite, const llvm::Function *Callee)
    : CallSite(CallSite), Callee(Callee) {
  for (unsigned Idx = 0; Idx < CallSite->arg_size(); ++Idx) {
    Actuals.push_back(CallSite->getArgOperand(Idx));
  }
  // Set up the formal parameters
  for (unsigned Idx = 0; Idx < Callee->arg_size(); ++Idx) {
    const auto *Frm = getNthFunctionArgument(Callee, Idx);
    assert(Frm && "Invalid formal");
    Formals.push_back(Frm);
  }
}
std::set<const llvm::Value *>
MapFactsToCalleeFlowFunction::computeTargets(const llvm::Value *Source) {
  std::set<const llvm::Value *> Res;
  // Handle C-style varargs functions
  if (Callee->isVarArg()) {
    const auto *VaListAlloca = getVaListTagOrNull(*Callee);
    // Map actual parameter into corresponding formal parameter.
    for (unsigned Idx = 0; Idx < Actuals.size(); ++Idx) {
      if (Source == Actuals[Idx] || (LLVMZeroValue::isLLVMZeroValue(Source) &&
                                     isConstant(Actuals[Idx]))) {
        if (Idx >= Callee->arg_size()) {
          if (VaListAlloca) {
            Res.insert(VaListAlloca);
          }
        } else {
          Res.insert(Formals[Idx]); // corresponding formal
        }
      }
    }
    if (LLVMZeroValue::isLLVMZeroValue(Source)) {
      Res.insert(Source);
    }
    return Res;
  }
  // Handle ordinary case
  // Map actual parameter into corresponding formal parameter.
  for (unsigned Idx = 0; Idx < Actuals.size(); ++Idx) {
    if (Source == Actuals[Idx] ||
        (LLVMZeroValue::isLLVMZeroValue(Source) && isConstant(Actuals[Idx]))) {
      Res.insert(Formals[Idx]); // corresponding formal
    }
  }
  if (LLVMZeroValue::isLLVMZeroValue(Source)) {
    Res.insert(Source);
  }
  return Res;
}

} // namespace psr::glca
