/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/TransferEdgeFunction.h"

#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/ByRef.h"

#include "llvm/IR/Instruction.h"

namespace psr::XTaint {

auto TransferEdgeFunction::computeTarget(ByConstRef<l_t> Source) const -> l_t {
  static_assert(IsEdgeFunction<TransferEdgeFunction>);
  assert(BBO != nullptr);
  if (const auto *Sani = Source.getSanitizer()) {
    if (!Load || BBO->mustComeBefore(Sani, Load)) {
      return To;
    }
  }
  if (Source.isSanitized()) {
    return To;
  }

  return nullptr;
}

bool operator==(const TransferEdgeFunction &LHS,
                const TransferEdgeFunction &RHS) noexcept {
  return LHS.To == RHS.To;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const TransferEdgeFunction &TRE) {
  return OS << "Transfer[To: " << llvmIRToShortString(TRE.To) << "]";
}

} // namespace psr::XTaint
