/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLERFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLERFLOWFUNCTION_H

#include <set>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace llvm {
class CallBase;
class Function;
class Instruction;
class ReturnInst;
class Value;
} // namespace llvm
namespace psr {

class MapFactsToCallerFlowFunction : public FlowFunction<const llvm::Value *> {
  std::vector<const llvm::Value *> Actuals;
  std::vector<const llvm::Value *> Formals;
  const llvm::CallBase *CallSite;
  const llvm::ReturnInst *ExitStmt;
  const llvm::Function *Callee;

public:
  MapFactsToCallerFlowFunction(const llvm::CallBase *CallSite,
                               const llvm::Instruction *ExitStmt,
                               const llvm::Function *Callee);
  std::set<const llvm::Value *>
  computeTargets(const llvm::Value *Source) override;
};

} // namespace psr

#endif
