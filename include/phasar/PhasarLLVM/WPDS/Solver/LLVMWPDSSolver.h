/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_SOLVER_LLVMWPDSSOLVER_H_
#define PHASAR_PHASARLLVM_WPDS_SOLVER_LLVMWPDSSOLVER_H_

#include <phasar/PhasarLLVM/WPDS/Solver/WPDSSolver.h>

namespace llvm {
class Instruction;
class Function;
} // namespace llvm

namespace psr {

template <typename D, typename V, typename I>
class LLVMWPDSSolver : psr::WPDSSolver<const llvm::Instruction *, D,
                                       const llvm::Function *, V, I> {};
} // namespace psr

#endif
