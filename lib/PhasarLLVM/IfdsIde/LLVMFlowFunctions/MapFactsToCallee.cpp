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
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>

#include <phasar/Utils/LLVMShorthands.h>

using namespace std;
using namespace psr;

namespace psr {

MapFactsToCallee::MapFactsToCallee(
    const llvm::ImmutableCallSite &callSite, const llvm::Function *destMthd,
    function<bool(const llvm::Value *)> predicate)
    : predicate(predicate) {
  // Set up the actual parameters
  for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
    actuals.push_back(callSite.getArgOperand(idx));
  }
  // Set up the formal parameters
  for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
    formals.push_back(getNthFunctionArgument(destMthd, idx));
  }
}

set<const llvm::Value *>
MapFactsToCallee::computeTargets(const llvm::Value *source) {
  if (!isLLVMZeroValue(source)) {
    set<const llvm::Value *> res;
    // Map actual parameter into corresponding formal parameter.
    for (unsigned idx = 0; idx < actuals.size(); ++idx) {
      if (source == actuals[idx] && predicate(actuals[idx])) {
        res.insert(formals[idx]); // corresponding formal
      }
    }
    return res;
  } else {
    return {source};
  }
}

} // namespace psr
