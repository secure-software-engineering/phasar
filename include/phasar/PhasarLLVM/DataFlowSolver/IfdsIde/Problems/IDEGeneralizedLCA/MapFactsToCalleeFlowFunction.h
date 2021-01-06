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

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Value.h"

namespace psr {

class MapFactsToCalleeFlowFunction : public FlowFunction<const llvm::Value *> {
  llvm::ImmutableCallSite cs;
  const llvm::Function *destMthd;
  std::vector<const llvm::Value *> actuals;
  std::vector<const llvm::Value *> formals;

public:
  MapFactsToCalleeFlowFunction(llvm::ImmutableCallSite cs,
                               const llvm::Function *destMthd);
  std::set<const llvm::Value *>
  computeTargets(const llvm::Value *source) override;
};

} // namespace psr

#endif
