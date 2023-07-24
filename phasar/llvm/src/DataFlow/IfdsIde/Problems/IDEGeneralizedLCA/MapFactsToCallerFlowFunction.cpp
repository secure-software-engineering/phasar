/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCallerFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace psr::glca {

MapFactsToCallerFlowFunction::MapFactsToCallerFlowFunction(
    const llvm::CallBase *CallSite, const llvm::Instruction *ExitStmt,
    const llvm::Function *Callee)
    : CallSite(CallSite), ExitStmt(llvm::cast<llvm::ReturnInst>(ExitStmt)),
      Callee(Callee) {
  for (unsigned Idx = 0; Idx < CallSite->arg_size(); ++Idx) {
    Actuals.push_back(CallSite->getArgOperand(Idx));
  }
  // Set up the formal parameters
  for (unsigned Idx = 0; Idx < Callee->arg_size(); ++Idx) {
    Formals.push_back(getNthFunctionArgument(Callee, Idx));
  }
}
std::set<const llvm::Value *>
MapFactsToCallerFlowFunction::computeTargets(const llvm::Value *Source) {
  // most copied from phasar
  std::set<const llvm::Value *> Res;
  // Handle C-style varargs functions
  if (Callee->isVarArg() && !Callee->isDeclaration()) {
    const llvm::Instruction *AllocVarArg = nullptr;
    // Find the allocation of %struct.__va_list_tag
    for (const auto &BB : *Callee) {
      for (const auto &I : BB) {
        if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
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
    if (Source != nullptr && Source == AllocVarArg &&
        Source->getType()->isPointerTy()) {
      for (unsigned Idx = Formals.size(); Idx < Actuals.size(); ++Idx) {
        Res.insert(Actuals[Idx]);
      }
    }
  }
  // Handle ordinary case
  // Map formal parameter into corresponding actual parameter.
  for (unsigned Idx = 0; Idx < Formals.size(); ++Idx) {
    if (Source == Formals[Idx] && Formals[Idx]->getType()->isPointerTy()) {
      Res.insert(Actuals[Idx]); // corresponding actual
    }
  }
  // Collect return value facts
  if (Source == ExitStmt->getReturnValue() ||
      (LLVMZeroValue::isLLVMZeroValue(Source) && ExitStmt->getReturnValue() &&
       isConstant(ExitStmt->getReturnValue()))) {
    Res.insert(CallSite);
  }
  return Res;
}

} // namespace psr::glca
