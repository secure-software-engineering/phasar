/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCalleeFlowFunction.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

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
    // Map actual parameter into corresponding formal parameter.
    for (unsigned Idx = 0; Idx < Actuals.size(); ++Idx) {
      if (Source == Actuals[Idx] ||
          (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
           isConstant(Actuals[Idx]))) {
        if (Idx >= Callee->arg_size() && !Callee->isDeclaration()) {
          // Over-approximate by trying to add the
          //   alloca [1 x %struct.__va_list_tag], align 16
          // to the results
          // find the allocated %struct.__va_list_tag and generate it
          for (const auto &BB : *Callee) {
            for (const auto &I : BB) {
              if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                if (Alloc->getAllocatedType()->isArrayTy() &&
                    Alloc->getAllocatedType()->getArrayNumElements() > 0 &&
                    Alloc->getAllocatedType()
                        ->getArrayElementType()
                        ->isStructTy() &&
                    Alloc->getAllocatedType()
                            ->getArrayElementType()
                            ->getStructName() == "struct.__va_list_tag") {
                  Res.insert(Alloc);
                }
              }
            }
          }
        } else {
          Res.insert(Formals[Idx]); // corresponding formal
        }
      }
    }
    if (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source)) {
      Res.insert(Source);
    }
    return Res;
  }
  // Handle ordinary case
  // Map actual parameter into corresponding formal parameter.
  for (unsigned Idx = 0; Idx < Actuals.size(); ++Idx) {
    if (Source == Actuals[Idx] ||
        (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
         isConstant(Actuals[Idx]))) {
      Res.insert(Formals[Idx]); // corresponding formal
    }
  }
  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source)) {
    Res.insert(Source);
  }
  return Res;
}

} // namespace psr
