#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFGProvider.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Value.h"

namespace psr {
class SparseLLVMControlFlow {
public:
  using n_t = const llvm::Instruction *;
  using v_t = const llvm::Value *;

  /// \brief Advances Succ to the unique next instruction that may use
  /// Fact, based on the given alias information. If Succ may use Fact itself,
  /// or if there are potentially many next users, returns Succ.
  [[nodiscard]] static n_t advanceToNextUser(n_t Succ, const auto &Fact,
                                             LLVMAliasInfoRef AI) {
    using psr::valueOf;
    static_assert(std::is_convertible_v<decltype(valueOf(Fact)), v_t>);
    return advanceToNextUserImpl(Succ, valueOf(Fact), AI);
  }

  /// \brief Whether Inst may use Val, based on the given alias information
  [[nodiscard]] static bool shouldKeepInst(n_t Inst, v_t Val,
                                           LLVMAliasInfoRef AI);

private:
  [[nodiscard]] static bool isNoopIntrinsic(n_t Inst) {
    // isAssumeLikeIntrinsic() alone is not enough: llvm.objectsize and
    // llvm.ptr_annotation derive their result from a pointer operand
    const auto *II = llvm::dyn_cast<llvm::IntrinsicInst>(Inst);
    return II && II->isAssumeLikeIntrinsic() && II->getType()->isVoidTy();
  }

  [[nodiscard]] static bool isExitInst(n_t Inst) {
    return llvm::isa<llvm::ReturnInst, llvm::ResumeInst, llvm::UnreachableInst>(
        Inst);
  }

  [[nodiscard]] static n_t advanceToNextUserImpl(n_t Succ, v_t Fact,
                                                 LLVMAliasInfoRef AI) {
    if (Succ == Fact || isExitInst(Succ) || isStartInst(Succ)) {
      return Succ;
    }
    if (llvm::isa<llvm::CallBase>(Succ) && !isNoopIntrinsic(Succ)) {
      if (llvm::isa<llvm::GlobalValue>(Fact)) {
        return Succ;
      }
    }

    return advanceToNextUserImplInternal(Succ, Fact, AI);
  }

  [[nodiscard]] static n_t advanceToNextUserImplInternal(n_t Succ, v_t Fact,
                                                         LLVMAliasInfoRef AI);
};
} // namespace psr
