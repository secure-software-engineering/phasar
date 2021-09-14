/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <iostream>

#include "llvm/IR/Instruction.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/TransferEdgeFunction.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::XTaint {
TransferEdgeFunction::TransferEdgeFunction(BasicBlockOrdering &BBO,
                                           const llvm::Instruction *Load,
                                           const llvm::Instruction *To)
    : EdgeFunctionBase(Kind::Transfer, BBO), Load(Load), To(To) {}

auto TransferEdgeFunction::computeTarget(l_t Source) -> l_t {

  if (const auto *Sani = Source.getSanitizer()) {
    if (!Load || BBO.mustComeBefore(Sani, Load)) {
      return To;
    }
  }
  if (Source.isSanitized()) {
    return To;
  }

  return nullptr;
}

llvm::hash_code TransferEdgeFunction::getHashCode() const {
  return llvm::hash_combine(To);
}

bool TransferEdgeFunction::equal_to(EdgeFunctionPtrType Other) const {
  if (auto *OtherTransfer = dynamic_cast<TransferEdgeFunction *>(&*Other)) {
    return To == OtherTransfer->To;
  }
  return false;
}

void TransferEdgeFunction::print(std::ostream &OS,
                                 [[maybe_unused]] bool IsForDebug) const {
  OS << "Transfer[To: " << llvmIRToShortString(To) << "]";
}
} // namespace psr::XTaint