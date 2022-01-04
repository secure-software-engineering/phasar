/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLEE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLEE_H

#include "../Stats/TraceStats.h"

#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Instruction.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/ExtendedValue.h"

namespace psr {

class MapTaintedValuesToCallee : public FlowFunction<ExtendedValue> {
public:
  MapTaintedValuesToCallee(const llvm::CallInst *CallInst,
                           const llvm::Function *DestFun, TraceStats &TStats,
                           const ExtendedValue &ZeroValue)
      : CallInst(CallInst), DestFun(DestFun), TStats(TStats),
        ZeroValue(ZeroValue) {}
  ~MapTaintedValuesToCallee() override = default;

  std::set<ExtendedValue> computeTargets(ExtendedValue Fact) override;

private:
  const llvm::CallInst *CallInst;
  const llvm::Function *DestFun;
  TraceStats &TStats;
  ExtendedValue ZeroValue;
};

} // namespace psr

#endif
