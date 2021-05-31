/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLEEFLOWFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLEEFLOWFUNCTION_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"

namespace llvm {
class CallBase;
class Function;
class Value;
} // namespace llvm

namespace psr {

class MapFactsToCalleeFlowFunction : public FlowFunction<const llvm::Value *> {
protected:
  const llvm::CallBase *CallSite;
  const llvm::Function *Callee;
  std::vector<const llvm::Value *> Actuals;
  std::vector<const llvm::Value *> Formals;

public:
  MapFactsToCalleeFlowFunction(const llvm::CallBase *CallSite,
                               const llvm::Function *Callee);
  std::set<const llvm::Value *>
  computeTargets(const llvm::Value *source) override;
};

} // namespace psr

#endif
