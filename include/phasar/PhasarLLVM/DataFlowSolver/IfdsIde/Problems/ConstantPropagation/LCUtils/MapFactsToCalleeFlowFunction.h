#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Value.h"

namespace psr::LCUtils {
class MapFactsToCalleeFlowFunction
    : public psr::FlowFunction<const llvm::Value *> {
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
} // namespace psr::LCUtils