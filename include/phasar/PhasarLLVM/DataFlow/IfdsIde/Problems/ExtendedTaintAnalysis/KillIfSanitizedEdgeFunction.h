/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_KILLIFSANITIZEDEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_KILLIFSANITIZEDEDGEFUNCTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"
#include "phasar/Utils/ByRef.h"

namespace psr::XTaint {

struct KillIfSanitizedEdgeFunction
    : EdgeFunctionBase<KillIfSanitizedEdgeFunction> {
  BasicBlockOrdering *BBO{};
  const llvm::Instruction *Load{};

  using l_t = EdgeDomain;

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> Source) const;

  [[nodiscard]] inline const llvm::Instruction *getLoad() const { return Load; }
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              ByConstRef<KillIfSanitizedEdgeFunction> KEF);

[[nodiscard]] bool
operator==(ByConstRef<KillIfSanitizedEdgeFunction> LHS,
           ByConstRef<KillIfSanitizedEdgeFunction> RHS) noexcept;

} // namespace psr::XTaint

#endif
