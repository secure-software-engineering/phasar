/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"

namespace psr {
LLVMBasedBackwardICFG::LLVMBasedBackwardICFG(LLVMBasedICFG *ForwardICFG)
    : LLVMBasedBackwardCFG(*ForwardICFG->getIRDB(),
                           ForwardICFG->getIgnoreDbgInstructions()),
      ForwardICFG(ForwardICFG) {
  assert(ForwardICFG != nullptr);
}

FunctionRange LLVMBasedBackwardICFG::getAllFunctionsImpl() const {
  return ForwardICFG->getAllFunctions();
}

auto LLVMBasedBackwardICFG::getFunctionImpl(llvm::StringRef Fun) const -> f_t {
  return ForwardICFG->getFunction(Fun);
}

bool LLVMBasedBackwardICFG::isIndirectFunctionCallImpl(n_t Inst) const {
  return ForwardICFG->isIndirectFunctionCall(Inst);
}

bool LLVMBasedBackwardICFG::isVirtualFunctionCallImpl(n_t Inst) const {
  return ForwardICFG->isVirtualFunctionCall(Inst);
}

auto LLVMBasedBackwardICFG::allNonCallStartNodesImpl() const
    -> std::vector<n_t> {
  return ForwardICFG->allNonCallStartNodes();
}

auto LLVMBasedBackwardICFG::getCalleesOfCallAtImpl(n_t Inst) const noexcept
    -> llvm::ArrayRef<f_t> {
  return ForwardICFG->getCalleesOfCallAt(Inst);
}

auto LLVMBasedBackwardICFG::getCallersOfImpl(f_t Fun) const noexcept
    -> llvm::ArrayRef<n_t> {
  return ForwardICFG->getCallersOf(Fun);
}

auto LLVMBasedBackwardICFG::getCallsFromWithinImpl(f_t Fun) const
    -> llvm::SmallVector<n_t> {
  return ForwardICFG->getCallsFromWithin(Fun);
}

auto LLVMBasedBackwardICFG::getReturnSitesOfCallAtImpl(n_t Inst) const
    -> llvm::SmallVector<n_t, 2> {
  return getSuccsOf(Inst);
}

void LLVMBasedBackwardICFG::printImpl(llvm::raw_ostream &OS) const {
  ForwardICFG->print(OS);
}

nlohmann::json LLVMBasedBackwardICFG::getAsJsonImpl() const {
  return ForwardICFG->getAsJson();
}
} // namespace psr
