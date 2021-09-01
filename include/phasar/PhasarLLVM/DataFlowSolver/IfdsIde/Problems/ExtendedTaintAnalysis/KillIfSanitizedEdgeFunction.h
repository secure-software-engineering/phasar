/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_KILLIFSANITIZEDEDGEFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_KILLIFSANITIZEDEDGEFUNCTION_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"

namespace psr::XTaint {

#if 1
class KillIfSanitizedEdgeFunction : public EdgeFunctionBase {
  const llvm::Instruction *Load;

public:
  KillIfSanitizedEdgeFunction(BasicBlockOrdering &BBO,
                              const llvm::Instruction *Load);

  l_t computeTarget(l_t Source) override;

  bool equal_to(EdgeFunctionPtrType OtherFunction) const override;

  void print(std::ostream &OS, bool IsForDebug = false) const override;

  inline const llvm::Instruction *getLoad() const { return Load; }

  llvm::hash_code getHashCode() const override;

  inline static bool classof(const EdgeFunctionBase *EF) {
    return EF->getKind() == Kind::KillIfSani;
  }
};

inline EdgeFunctionBase::EdgeFunctionPtrType
makeKillIfSanitizedEdgeFunction(BasicBlockOrdering &BBO,
                                const llvm::Instruction *Load) {
  return EdgeFunctionBase::EdgeFunctionPtrType(
      new KillIfSanitizedEdgeFunction(BBO, Load));
}
#else
using KillIfSanitizedEdgeFunction = psr::EdgeIdentity<EdgeDomain>;

inline EdgeFunctionBase::EdgeFunctionPtrType
makeKillIfSanitizedEdgeFunction(BasicBlockOrdering &BBO,
                                const llvm::Instruction *Load) {
  return EdgeIdentity<EdgeDomain>::getInstance();
}
#endif
} // namespace psr::XTaint

#endif // PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_KILLIFSANITIZEDEDGEFUNCTION_H_