/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/KillIfSanitizedEdgeFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"

#include "llvm/IR/Instruction.h"

namespace psr::XTaint {

EdgeDomain
KillIfSanitizedEdgeFunction::computeTarget(ByConstRef<l_t> Source) const {
  static_assert(IsEdgeFunction<KillIfSanitizedEdgeFunction>);
  assert(BBO != nullptr);
  if (const auto *Sani = Source.getSanitizer()) {
    if (!Load) {
      return Sanitized{};
    }
    if (Sani->getFunction() == Load->getFunction() &&
        BBO->mustComeBefore(Sani, Load)) {
      return Sanitized{};
    }

    return nullptr;
  }

  return Source;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              ByConstRef<KillIfSanitizedEdgeFunction> KEF) {
  return OS << "KillIfSani[" << &KEF << "]";
}

[[nodiscard]] bool
operator==(ByConstRef<KillIfSanitizedEdgeFunction> LHS,
           ByConstRef<KillIfSanitizedEdgeFunction> RHS) noexcept {
  // Assume, the Analysis to be the same
  return LHS.Load == RHS.Load;
}

} // namespace psr::XTaint
