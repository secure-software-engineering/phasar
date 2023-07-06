/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLEE_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MAPTAINTEDVALUESTOCALLEE_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde//IFDSFieldSensTaintAnalysis/Utils/ExtendedValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"

#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Instruction.h"

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
