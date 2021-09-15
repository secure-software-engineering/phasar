#include <array>

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"

namespace psr {

llvm::DominatorTree &BasicBlockOrdering::getDom(const llvm::Function *F) {
  auto &Ret = Dom[F];
  if (!Ret) {
    Ret =
        std::make_unique<llvm::DominatorTree>(*const_cast<llvm::Function *>(F));
  }
  return *Ret;
}

bool BasicBlockOrdering::mustComeBefore(const llvm::Instruction *LHS,
                                        const llvm::Instruction *RHS) {
  assert(LHS);
  assert(RHS);

  if (LHS->getFunction() != RHS->getFunction()) {
    // We don't want the complexity of computing a dominator tree over the
    // call-graph...
    return false;
  }

  if (LHS->getParent() == RHS->getParent()) {
    return LHS->comesBefore(RHS);
  }

  return getDom(LHS->getFunction()).dominates(LHS, RHS);
}

} // namespace psr