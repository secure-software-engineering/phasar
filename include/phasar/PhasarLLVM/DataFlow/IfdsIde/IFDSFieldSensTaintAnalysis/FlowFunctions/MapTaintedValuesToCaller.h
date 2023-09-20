/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLER_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLER_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde//IFDSFieldSensTaintAnalysis/Utils/ExtendedValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"

#include "llvm/IR/Instructions.h"

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
