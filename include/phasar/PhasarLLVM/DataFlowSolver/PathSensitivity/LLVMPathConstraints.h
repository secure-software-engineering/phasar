/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_LLVMPATHCONSTRAINTS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_LLVMPATHCONSTRAINTS_H

#include "z3++.h"

#include <optional>

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr {
class LLVMPathConstraints {
  /// TODO: implement
public:
  z3::context &getContext() noexcept { return Z3Context; }

  std::optional<z3::expr> getConstraintFromEdge(const llvm::Instruction *Curr,
                                                const llvm::Instruction *Succ);

private:
  z3::context Z3Context;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_LLVMPATHCONSTRAINTS_H
