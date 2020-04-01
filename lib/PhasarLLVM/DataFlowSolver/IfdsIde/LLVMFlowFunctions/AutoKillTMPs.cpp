/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/AutoKillTMPs.h"

using namespace std;
using namespace psr;

namespace psr {

AutoKillTMPs::AutoKillTMPs(
    std::shared_ptr<FlowFunction<const llvm::Value *>> FF,
    const llvm::Instruction *In)
    : delegate(FF), inst(In) {}

std::set<const llvm::Value *>
AutoKillTMPs::computeTargets(const llvm::Value *Source) {
  std::set<const llvm::Value *> result = delegate->computeTargets(Source);
  for (const llvm::Use &u : inst->operands()) {
    if (llvm::isa<llvm::LoadInst>(u)) {
      result.erase(u);
    }
  }
  return result;
}

} // namespace psr
