/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_GENEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_GENEDGEFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr::XTaint {
class GenEdgeFunction : public EdgeFunctionBase {
  const llvm::Instruction *Sani;

public:
  GenEdgeFunction(BasicBlockOrdering &BBO, const llvm::Instruction *Sani);

  l_t computeTarget(l_t Source) override;

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override;

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override;

  bool equal_to(EdgeFunctionPtrType OtherFunction) const override;

  void print(std::ostream &OS, bool IsForDebug = false) const override;

  inline const llvm::Instruction *getSanitizer() const { return Sani; }

  static inline bool classof(const EdgeFunctionBase *EF) {
    return EF->getKind() == EFKind::Gen;
  }

  llvm::hash_code getHashCode() const override;
};
} // namespace psr::XTaint

#endif
