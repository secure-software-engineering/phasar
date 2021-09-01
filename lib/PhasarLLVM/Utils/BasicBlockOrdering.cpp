#include <array>

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"

namespace psr {

llvm::DominatorTree &BasicBlockOrdering::getDom(const llvm::Function *F) {
  auto &ret = Dom[F];
  if (!ret) {
    ret =
        std::make_unique<llvm::DominatorTree>(*const_cast<llvm::Function *>(F));
  }
  return *ret;
}

unsigned BasicBlockOrdering::getOrder(const llvm::Instruction *I) {
  assert(I);
  llvm::SmallVector<const llvm::Instruction *, 4> cache;
  while (I && !InstOrder.count(I)) {
    cache.push_back(I);
    I = I->getPrevNode();
  }

  unsigned ctr = I ? InstOrder[I] : 0;
  for (size_t i = cache.size(); i > 0; --i) {
    InstOrder[cache[i - 1]] = ++ctr;
  }
  return ctr;
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

  if (LHS->getParent() == RHS->getParent())
    // Since LLVM-11: LHS->comesBefore(RHS);
    return getOrder(LHS) < getOrder(RHS);
  else
    return getDom(LHS->getFunction()).dominates(LHS, RHS);
}

void BasicBlockOrdering::clear() { InstOrder.shrink_and_clear(); }
} // namespace psr