/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsAlongsideCallSite.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>

using namespace psr;

namespace psr {

MapFactsAlongsideCallSite::MapFactsAlongsideCallSite(
    llvm::ImmutableCallSite CallSite,
    std::function<bool(llvm::ImmutableCallSite, const llvm::Value *)> Predicate)
    : CallSite(CallSite), Predicate(Predicate) {}

std::set<const llvm::Value *>
MapFactsAlongsideCallSite::computeTargets(const llvm::Value *Source) {
  // always propagate the zero fact
  if (!LLVMZeroValue::getInstance()->isLLVMZeroValue(Source)) {
    return {Source};
  }
  // propagate if predicate does not hold, i.e. fact is not involved in the call
  if (!Predicate(CallSite, Source)) {
    return {Source};
  }
  // otherwise kill fact
  return {};
}

} // namespace psr
