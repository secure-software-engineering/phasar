/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCallerFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"

namespace psr {

MapFactsToCallerFlowFunction::MapFactsToCallerFlowFunction(
    llvm::ImmutableCallSite Cs, const llvm::Instruction *ExitStmt,
    const llvm::Function *CalleeMthd)
    : cs(Cs), exitStmt(llvm::cast<llvm::ReturnInst>(ExitStmt)),
      calleeMthd(CalleeMthd) {
  for (unsigned Idx = 0; Idx < Cs.getNumArgOperands(); ++Idx) {
    actuals.push_back(Cs.getArgOperand(Idx));
  }
  // Set up the formal parameters
  for (unsigned Idx = 0; Idx < CalleeMthd->arg_size(); ++Idx) {
    formals.push_back(getNthFunctionArgument(CalleeMthd, Idx));
  }
}
std::set<const llvm::Value *>
MapFactsToCallerFlowFunction::computeTargets(const llvm::Value *Source) {
  // most copied from phasar
  std::set<const llvm::Value *> Res;
  // Handle C-style varargs functions
  if (calleeMthd->isVarArg() && !calleeMthd->isDeclaration()) {
    const llvm::Instruction *AllocVarArg;
    // Find the allocation of %struct.__va_list_tag
    for (auto &BB : *calleeMthd) {
      for (auto &I : BB) {
        if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
          if (Alloc->getAllocatedType()->isArrayTy() &&
              Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
              Alloc->getAllocatedType()->getArrayElementType()->isStructTy() &&
              Alloc->getAllocatedType()
                      ->getArrayElementType()
                      ->getStructName() == "struct.__va_list_tag") {
            AllocVarArg = Alloc;
            // TODO break out this nested loop earlier (without goto ;-)
          }
        }
      }
    }
    // Generate the varargs things by using an over-approximation
    if (Source == AllocVarArg && Source->getType()->isPointerTy()) {
      for (unsigned Idx = formals.size(); Idx < actuals.size(); ++Idx) {
        Res.insert(actuals[Idx]);
      }
    }
  }
  // Handle ordinary case
  // Map formal parameter into corresponding actual parameter.
  for (unsigned Idx = 0; Idx < formals.size(); ++Idx) {
    if (Source == formals[Idx] && formals[Idx]->getType()->isPointerTy()) {
      Res.insert(actuals[Idx]); // corresponding actual
    }
  }
  // Collect return value facts
  if (Source == exitStmt->getReturnValue() ||
      (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
       exitStmt->getReturnValue() && isConstant(exitStmt->getReturnValue()))) {
    Res.insert(cs.getInstruction());
  }
  return Res;
}

} // namespace psr
