/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_DEBUGEDGEIDENTITY_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_DEBUGEDGEIDENTITY_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"

namespace llvm {
class Instruction;
} // namespace llvm

namespace psr::XTaint {
class DebugEdgeIdentity
    : public EdgeFunction<EdgeDomain>,
      public std::enable_shared_from_this<DebugEdgeIdentity> {
  const llvm::Instruction *Inst;

public:
  using typename EdgeFunction<EdgeDomain>::EdgeFunctionPtrType;
  DebugEdgeIdentity(const llvm::Instruction *Inst);

  EdgeDomain computeTarget(EdgeDomain Source) override;

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override;

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override;

  bool equal_to(EdgeFunctionPtrType Other) const override;

  void print(llvm::raw_ostream &OS, bool IsForDebug = false) const override;
};
} // namespace psr::XTaint

#endif
