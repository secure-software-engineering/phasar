/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLERFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_MAPFACTSTOCALLERFLOWFUNCTION_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include <set>
#include <vector>

namespace llvm {
class CallBase;
class Function;
class Instruction;
class ReturnInst;
class Value;
} // namespace llvm
namespace psr::glca {

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

} // namespace psr::glca

#endif
