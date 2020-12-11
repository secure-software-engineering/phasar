/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCalleeFlowFunction.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

MapFactsToCalleeFlowFunction::MapFactsToCalleeFlowFunction(
    llvm::ImmutableCallSite Cs, const llvm::Function *DestMthd)
    : cs(Cs), destMthd(DestMthd) {

  for (unsigned Idx = 0; Idx < Cs.getNumArgOperands(); ++Idx) {
    actuals.push_back(Cs.getArgOperand(Idx));
  }
  // Set up the formal parameters
  for (unsigned Idx = 0; Idx < DestMthd->arg_size(); ++Idx) {
    auto Frm = getNthFunctionArgument(DestMthd, Idx);
    assert(Frm && "Invalid formal");
    formals.push_back(Frm);
  }
}
std::set<const llvm::Value *>
MapFactsToCalleeFlowFunction::computeTargets(const llvm::Value *Source) {
  // most copied from phasar
  // if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(source)) {
  std::set<const llvm::Value *> Res;
  // Handle C-style varargs functions
  if (destMthd->isVarArg()) {
    // Map actual parameter into corresponding formal parameter.
    for (unsigned Idx = 0; Idx < actuals.size(); ++Idx) {
      if (Source == actuals[Idx] ||
          (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
           isConstant(actuals[Idx]))) {
        if (Idx >= destMthd->arg_size() && !destMthd->isDeclaration()) {
          // Over-approximate by trying to add the
          //   alloca [1 x %struct.__va_list_tag], align 16
          // to the results
          // find the allocated %struct.__va_list_tag and generate it
          for (auto &BB : *destMthd) {
            for (auto &I : BB) {
              if (auto Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
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
    if (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source))
      Res.insert(Source);
    return Res;
  } else {
    // Handle ordinary case
    // Map actual parameter into corresponding formal parameter.
    for (unsigned Idx = 0; Idx < actuals.size(); ++Idx) {
      if (Source == actuals[Idx] ||
          (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source) &&
           isConstant(actuals[Idx]))) {
        Res.insert(formals[Idx]); // corresponding formal
        // std::cout << "Map actual to formal: " < < < < std::endl;
        // llvm::outs() << "Map actual " << *actuals[idx] << " to formal "
        //            << *formals[idx] << "\n";
      }
    }
    if (LLVMZeroValue::getInstance()->isLLVMZeroValue(Source))
      Res.insert(Source);
    return Res;
  }
}

} // namespace psr
