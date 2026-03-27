#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel, Eric Bodden.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionCompressor.h"
#include "phasar/Utils/SCCGeneric.h"

#include "llvm/IR/Instruction.h"

#include <iterator>

namespace psr {

struct ControlFlowOrder {
  enum class CFGOrderId : uint32_t {};

  Compressor<const llvm::Instruction *, CFGOrderId> Order;

  [[nodiscard]] auto begin() const noexcept {
    return std::make_reverse_iterator(Order.begin());
  }
  [[nodiscard]] auto end() const noexcept {
    return std::make_reverse_iterator(Order.end());
  }
};

// TODO: Make ehtis independent from LLVM IR
void computeCFGOrder(ControlFlowOrder &Into, const llvm::Function *Fun);
void computeCFGOrder(
    ControlFlowOrder &Into, const SCCHolder<FunctionId> &SCCs,
    SCCId<FunctionId> SCC, const psr::LLVMBasedCallGraph &CG,
    const Compressor<const llvm::Function *, FunctionId> &Functions);

[[nodiscard]] inline ControlFlowOrder
computeCFGOrder(const llvm::Function *Fun) {
  ControlFlowOrder Ret;
  computeCFGOrder(Ret, Fun);
  return Ret;
}
} // namespace psr
