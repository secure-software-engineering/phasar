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
#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FunctionExtras.h"

#include "phasar/DB/ProjectIRDB.h"

namespace llvm {
class Function;
class BasicBlock;
class Instruction;
class DominatorTree;
} // namespace llvm

namespace psr {

class DefaultDominatorTreeAnalysis {
  llvm::DenseMap<const llvm::Function *, std::unique_ptr<llvm::DominatorTree>>
      Dom;

public:
  llvm::DominatorTree &operator()(const llvm::Function *F);
};

/// Provides a simple partial ordering of BasicBlocks based on LLVM's
/// DominatorTree.
class BasicBlockOrdering {
  /// Note: Cannot use std::function, because we need to support move-only
  /// functors(e.g. DefaultDominatorTreeAnalysis)
  llvm::unique_function<llvm::DominatorTree &(const llvm::Function *)>
      getDom; // NOLINT

public:
  template <
      typename DTA,
      typename = std::enable_if_t<!std::is_same_v<
          BasicBlockOrdering, std::remove_reference_t<std::decay_t<DTA>>>>>
  explicit BasicBlockOrdering(DTA &&Dta) : getDom(std::forward<DTA>(Dta)) {}

  bool mustComeBefore(const llvm::Instruction *LHS,
                      const llvm::Instruction *RHS);
};
} // namespace psr

#endif
