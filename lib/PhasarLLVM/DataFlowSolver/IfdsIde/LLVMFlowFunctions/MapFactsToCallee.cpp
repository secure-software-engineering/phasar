/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <utility>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"

#include "phasar/Utils/LLVMShorthands.h"

using namespace std;
using namespace psr;

namespace psr {

MapFactsToCallee::MapFactsToCallee(
    llvm::ImmutableCallSite CallSite, const llvm::Function *DestFun,
    function<bool(const llvm::Value *)> Predicate)
    : destFun(DestFun), predicate(std::move(Predicate)) {
  // Set up the actual parameters
  for (unsigned Idx = 0; Idx < CallSite.getNumArgOperands(); ++Idx) {
    actuals.push_back(CallSite.getArgOperand(Idx));
  }
  // Set up the formal parameters
  for (unsigned Idx = 0; Idx < destFun->arg_size(); ++Idx) {
    formals.push_back(getNthFunctionArgument(destFun, Idx));
  }
}

set<const llvm::Value *>
MapFactsToCallee::computeTargets(const llvm::Value *Source) {
  if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(Source)) {
    set<const llvm::Value *> Res;
    // Handle C-style varargs functions
    if (destFun->isVarArg()) {
      // Map actual parameter into corresponding formal parameter.
      for (unsigned Idx = 0; Idx < actuals.size(); ++Idx) {
        if (Source == actuals[Idx] && predicate(actuals[Idx])) {
          if (Idx >= destFun->arg_size() && !destFun->isDeclaration()) {
            // Over-approximate by trying to add the
            //   alloca [1 x %struct.__va_list_tag], align 16
            // to the results
            // find the allocated %struct.__va_list_tag and generate it
            for (const auto &BB : *destFun) {
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
            Res.insert(formals[Idx]); // corresponding formal
          }
        }
      }
      return Res;
    } else {
      // Handle ordinary case
      // Map actual parameter into corresponding formal parameter.
      for (unsigned Idx = 0; Idx < actuals.size(); ++Idx) {
        if (Source == actuals[Idx] && predicate(actuals[Idx])) {
          Res.insert(formals[Idx]); // corresponding formal
        }
      }
      return Res;
    }
  } else {
    return {Source};
  }
}

} // namespace psr
