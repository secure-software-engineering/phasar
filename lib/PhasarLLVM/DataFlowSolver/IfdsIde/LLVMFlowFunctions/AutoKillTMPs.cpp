/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/AutoKillTMPs.h>

using namespace std;
using namespace psr;

namespace psr {

AutoKillTMPs::AutoKillTMPs(
    std::shared_ptr<FlowFunction<const llvm::Value *>> ff,
    const llvm::Instruction *in)
    : delegate(ff), inst(in) {}

std::set<const llvm::Value *>
AutoKillTMPs::computeTargets(const llvm::Value *source) {
  std::set<const llvm::Value *> result = delegate->computeTargets(source);
  for (const llvm::Use &u : inst->operands()) {
    if (llvm::isa<llvm::LoadInst>(u)) {
      result.erase(u);
    }
  }
  return result;
}

} // namespace psr
