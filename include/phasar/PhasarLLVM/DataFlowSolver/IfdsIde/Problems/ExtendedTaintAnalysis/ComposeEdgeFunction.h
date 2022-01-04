/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_COMPOSEEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_COMPOSEEDGEFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

namespace psr::XTaint {
class ComposeEdgeFunction : public EdgeFunctionBase {
  EdgeFunctionPtrType F, G;

public:
  ComposeEdgeFunction(BasicBlockOrdering &BBO, EdgeFunctionPtrType F,
                      EdgeFunctionPtrType G);

  l_t computeTarget(l_t Source) override;

  bool equal_to(EdgeFunctionPtrType Other) const override;

  void print(std::ostream &OS, bool IsForDebug = false) const override;

  llvm::hash_code getHashCode() const override;

  static inline bool classof(const EdgeFunctionBase *EF) {
    return EF->getKind() == EFKind::Compose;
  }
};
} // namespace psr::XTaint

#endif
