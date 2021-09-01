/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_BASICBLOCKORDERING_H_
#define PHASAR_PHASARLLVM_UTILS_BASICBLOCKORDERING_H_

#include <memory>

#include "llvm/ADT/DenseMap.h"

#include "phasar/DB/ProjectIRDB.h"

namespace llvm {
class Function;
class BasicBlock;
class Instruction;
class DominatorTree;
} // namespace llvm

namespace psr {
/// Provides a simple partial ordering of BasicBlocks based on LLVM's
/// DominatorTree.
class BasicBlockOrdering {
  llvm::DenseMap<const llvm::Function *, std::unique_ptr<llvm::DominatorTree>>
      Dom;
  // InstOrder is only used for compatibility with LLVM-10 or lower
  llvm::DenseMap<const llvm::Instruction *, unsigned> InstOrder;

  llvm::DominatorTree &getDom(const llvm::Function *F);
  unsigned getOrder(const llvm::Instruction *I);

public:
  // bool mustComeBefore(const llvm::BasicBlock *LHS, const llvm::BasicBlock
  // *RHS);
  bool mustComeBefore(const llvm::Instruction *LHS,
                      const llvm::Instruction *RHS);
  void clear();
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_BASICBLOCKORDERING_H_