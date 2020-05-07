/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>
#include <utility>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"

#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

MapFactsToCaller::MapFactsToCaller(
    llvm::ImmutableCallSite Cs, const llvm::Function *CalleeFun,
    const llvm::Instruction *ExitStmt,
    function<bool(const llvm::Value *)> ParamPredicate,
    function<bool(const llvm::Function *)> ReturnPredicate)
    : callSite(Cs), calleeFun(CalleeFun),
      exitStmt(llvm::dyn_cast<llvm::ReturnInst>(ExitStmt)),
      paramPredicate(std::move(ParamPredicate)),
      returnPredicate(std::move(ReturnPredicate)) {
  // Set up the actual parameters
  for (unsigned Idx = 0; Idx < callSite.getNumArgOperands(); ++Idx) {
    actuals.push_back(callSite.getArgOperand(Idx));
  }
  // Set up the formal parameters
  for (unsigned Idx = 0; Idx < calleeFun->arg_size(); ++Idx) {
    formals.push_back(getNthFunctionArgument(calleeFun, Idx));
  }
}

set<const llvm::Value *>
MapFactsToCaller::computeTargets(const llvm::Value *Source) {
  if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(Source)) {
    set<const llvm::Value *> Res;
    // Handle C-style varargs functions
    if (calleeFun->isVarArg() && !calleeFun->isDeclaration()) {
      const llvm::Instruction *AllocVarArg;
      // Find the allocation of %struct.__va_list_tag
      for (const auto &BB : *calleeFun) {
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
              AllocVarArg = Alloc;
              // TODO break out this nested loop earlier (without goto ;-)
            }
          }
        }
      }
      // Generate the varargs things by using an over-approximation
      if (Source == AllocVarArg) {
        for (unsigned Idx = formals.size(); Idx < actuals.size(); ++Idx) {
          Res.insert(actuals[Idx]);
        }
      }
    }
    // Handle ordinary case
    // Map formal parameter into corresponding actual parameter.
    for (unsigned Idx = 0; Idx < formals.size(); ++Idx) {
      if (Source == formals[Idx] && paramPredicate(formals[Idx])) {
        Res.insert(actuals[Idx]); // corresponding actual
      }
    }
    // Collect return value facts
    if (Source == exitStmt->getReturnValue() && returnPredicate(calleeFun)) {
      Res.insert(callSite.getInstruction());
    }
    return Res;
  } else {
    return {Source};
  }
}

} // namespace psr
