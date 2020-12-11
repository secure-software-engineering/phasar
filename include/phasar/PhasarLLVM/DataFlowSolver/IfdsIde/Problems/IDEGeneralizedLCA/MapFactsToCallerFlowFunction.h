/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLERFLOWFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLERFLOWFUNCTION_H_

#include <set>
#include <vector>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

class MapFactsToCallerFlowFunction : public FlowFunction<const llvm::Value *> {
  std::vector<const llvm::Value *> actuals;
  std::vector<const llvm::Value *> formals;
  llvm::ImmutableCallSite cs;
  const llvm::ReturnInst *exitStmt;
  const llvm::Function *calleeMthd;

public:
  MapFactsToCallerFlowFunction(llvm::ImmutableCallSite cs,
                               const llvm::Instruction *exitStmt,
                               const llvm::Function *calleeMthd);
  std::set<const llvm::Value *>
  computeTargets(const llvm::Value *source) override;
};

} // namespace psr

#endif
