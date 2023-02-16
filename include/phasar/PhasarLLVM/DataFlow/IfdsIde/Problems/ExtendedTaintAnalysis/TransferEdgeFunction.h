/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_TRANSFEREDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_TRANSFEREDGEFUNCTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

namespace psr::XTaint {
class TransferEdgeFunction : public EdgeFunctionBase {
  const llvm::Instruction *Load;
  const llvm::Instruction *To;

public:
  TransferEdgeFunction(BasicBlockOrdering &BBO, const llvm::Instruction *Load,
                       const llvm::Instruction *To);

  l_t computeTarget(l_t Source) override;

  llvm::hash_code getHashCode() const override;

  bool equal_to(EdgeFunctionPtrType Other) const override;

  void print(llvm::raw_ostream &OS, bool IsForDebug = false) const override;
};
} // namespace psr::XTaint

#endif
