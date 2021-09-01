/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_JOINCONSTEDGEFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_JOINCONSTEDGEFUNCTION_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

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

  void print(std::ostream &OS, bool IsForDebug = false) const override;

  inline const llvm::Instruction *getConstant() const { return OtherConst; }

  inline const EdgeFunctionPtrType &getFunction() const { return OtherFn; }

  llvm::hash_code getHashCode() const override;

  inline static bool classof(const EdgeFunctionBase *EF) {
    return EF->getKind() == Kind::JoinConst;
  }
};
} // namespace psr::XTaint

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_JOINCONSTEDGEFUNCTION_H_