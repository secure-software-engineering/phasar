/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_JOINCONSTEDGEFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_JOINCONSTEDGEFUNCTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

namespace psr::XTaint {
class JoinConstEdgeFunction : public EdgeFunctionBase {
  EdgeFunctionPtrType OtherFn;
  const llvm::Instruction *OtherConst;

public:
  JoinConstEdgeFunction(BasicBlockOrdering &BBO, EdgeFunctionPtrType OtherFn,
                        const llvm::Instruction *OtherConst);

  l_t computeTarget(l_t Source) override;

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override;

  bool equal_to(EdgeFunctionPtrType OtherFunction) const override;

  void print(llvm::raw_ostream &OS, bool IsForDebug = false) const override;

  inline const llvm::Instruction *getConstant() const { return OtherConst; }

  inline const EdgeFunctionPtrType &getFunction() const { return OtherFn; }

  llvm::hash_code getHashCode() const override;

  inline static bool classof(const EdgeFunctionBase *EF) {
    return EF->getKind() == EFKind::JoinConst;
  }
};
} // namespace psr::XTaint

#endif
