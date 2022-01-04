/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLER_H

#include "../Stats/TraceStats.h"

#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/ExtendedValue.h"

namespace psr {

class MapTaintedValuesToCaller : public FlowFunction<ExtendedValue> {
public:
  MapTaintedValuesToCaller(const llvm::CallInst *CallInst,
                           const llvm::ReturnInst *RetInst,
                           TraceStats &TraceStats, const ExtendedValue &ZV)
      : CallInst(CallInst), RetInst(RetInst), TraceStats(TraceStats), ZV(ZV) {}
  ~MapTaintedValuesToCaller() override = default;

  std::set<ExtendedValue> computeTargets(ExtendedValue Fact) override;

private:
  const llvm::CallInst *CallInst;
  const llvm::ReturnInst *RetInst;
  TraceStats &TraceStats;
  ExtendedValue ZV;
};

} // namespace psr

#endif
