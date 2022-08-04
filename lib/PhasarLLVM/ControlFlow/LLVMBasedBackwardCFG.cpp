/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h"
#include "phasar/DB/ProjectIRDB.h"

#include "llvm/IR/Instructions.h"

namespace psr {

LLVMBasedBackwardCFG::LLVMBasedBackwardCFG(bool IgnoreDbgInstructions) noexcept
    : ForwardCFG(IgnoreDbgInstructions) {}
LLVMBasedBackwardCFG::LLVMBasedBackwardCFG(const ProjectIRDB &IRDB,
                                           bool IgnoreDbgInstructions)
    : LLVMBasedBackwardCFG(IgnoreDbgInstructions) {
  /// Initialize BackwardRets

  for (const auto *Fun : IRDB.getAllFunctions()) {
    auto &Ctx = Fun->getParent()->getContext();
    auto *Ret = llvm::ReturnInst::Create(Ctx);
    BackwardRets[Fun] = Ret;
    BackwardRetToFunction[Ret] = Fun;
  }
}

auto LLVMBasedBackwardCFG::getFunctionOfImpl(n_t Inst) const noexcept -> f_t {
  if (const auto *Fun = BackwardRetToFunction.lookup(Inst)) {
    return Fun;
  }
  return ForwardCFG.getFunctionOf(Inst);
}

auto LLVMBasedBackwardCFG::getPredsOfImpl(n_t Inst) const
    -> llvm::SmallVector<n_t, 2> {
  if (BackwardRetToFunction.count(Inst)) {
    return ForwardCFG.getStartPointsOf(ForwardCFG.getFunctionOf(Inst));
  }
  return ForwardCFG.getSuccsOf(Inst);
}

auto LLVMBasedBackwardCFG ::getSuccsOfImpl(n_t Inst) const
    -> llvm::SmallVector<n_t, 2> {
  if (BackwardRetToFunction.count(Inst)) {
    return {};
  }

  auto Ret = ForwardCFG.getPredsOf(Inst);
  if (Ret.empty()) {
    if (const auto *BRet =
            BackwardRets.lookup(ForwardCFG.getFunctionOf(Inst))) {
      Ret.push_back(BRet);
    }
  }
  return Ret;
}

auto LLVMBasedBackwardCFG ::getAllControlFlowEdgesImpl(f_t Fun) const
    -> std::vector<std::pair<n_t, n_t>> {
  std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
      Edges;

  for (const auto &I : llvm::instructions(Fun)) {
    if (ForwardCFG.IgnoreDbgInstructions) {
      // Check for call to intrinsic debug function
      if (const auto *DbgCallInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
        if (DbgCallInst->getCalledFunction() &&
            DbgCallInst->getCalledFunction()->isIntrinsic() &&
            (DbgCallInst->getCalledFunction()->getName() ==
             "llvm.dbg.declare")) {
          continue;
        }
      }
    }

    auto Successors = getSuccsOf(&I);
    for (const auto *Successor : Successors) {
      Edges.emplace_back(Successor, &I);
    }
  }

  return Edges;
}

auto LLVMBasedBackwardCFG ::getStartPointsOfImpl(f_t Fun) const
    -> llvm::SmallVector<n_t, 2> {
  return ForwardCFG.getExitPointsOf(Fun);
}

auto LLVMBasedBackwardCFG ::getExitPointsOfImpl(f_t Fun) const
    -> llvm::SmallVector<n_t, 1> {
  return BackwardRets.empty()
             ? ForwardCFG.getStartPointsOf(Fun)
             : llvm::SmallVector<n_t, 1>{BackwardRets.lookup(Fun)};
}

bool LLVMBasedBackwardCFG ::isExitInstImpl(n_t Inst) const noexcept {
  return BackwardRetToFunction.empty() ? ForwardCFG.isStartPoint(Inst)
                                       : BackwardRetToFunction.count(Inst);
}

bool LLVMBasedBackwardCFG ::isStartPointImpl(n_t Inst) const noexcept {
  return ForwardCFG.isExitInst(Inst);
}

bool LLVMBasedBackwardCFG ::isFallThroughSuccessorImpl(
    n_t Inst, n_t Succ) const noexcept {
  assert(false && "FallThrough not valid in LLVM IR");
  return false;
}
bool LLVMBasedBackwardCFG ::isBranchTargetImpl(n_t Inst,
                                               n_t Succ) const noexcept {
  if (const auto *B = llvm::dyn_cast<llvm::BranchInst>(Succ)) {
    for (const auto *BB : B->successors()) {
      if (Inst == &(BB->front())) {
        return true;
      }
    }
  }
  return false;
}

} // namespace psr
