/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>

#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

namespace psr {

MapFactsToCaller::MapFactsToCaller(
    llvm::ImmutableCallSite cs, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitstmt,
    function<bool(const llvm::Value *)> paramPredicate,
    function<bool(const llvm::Function *)> returnPredicate)
    : callSite(cs), calleeMthd(calleeMthd),
      exitStmt(llvm::dyn_cast<llvm::ReturnInst>(exitstmt)),
      paramPredicate(paramPredicate), returnPredicate(returnPredicate) {
  // Set up the actual parameters
  for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
    actuals.push_back(callSite.getArgOperand(idx));
  }
  // Set up the formal parameters
  for (unsigned idx = 0; idx < calleeMthd->arg_size(); ++idx) {
    formals.push_back(getNthFunctionArgument(calleeMthd, idx));
  }
}

set<const llvm::Value *>
MapFactsToCaller::computeTargets(const llvm::Value *source) {
  if (!isLLVMZeroValue(source)) {
    set<const llvm::Value *> res;
    // Map formal parameter into corresponding actual parameter.
    for (unsigned idx = 0; idx < formals.size(); ++idx) {
      if (source == formals[idx] && paramPredicate(formals[idx])) {
        res.insert(actuals[idx]); // corresponding actual
      }
    }
    // Collect return value facts
    if (source == exitStmt->getReturnValue() && returnPredicate(calleeMthd)) {
      res.insert(callSite.getInstruction());
    }
    return res;
  } else {
    return {source};
  }
}

} // namespace psr
