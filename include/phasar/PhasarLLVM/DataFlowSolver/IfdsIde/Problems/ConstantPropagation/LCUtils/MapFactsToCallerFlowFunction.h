#pragma once

#include <set>
#include <vector>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::LCUtils {
class MapFactsToCallerFlowFunction
    : public psr::FlowFunction<const llvm::Value *> {
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
} // namespace psr::LCUtils