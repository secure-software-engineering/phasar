/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_FLOWFUNCTIONBASE_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_FLOWFUNCTIONBASE_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde//IFDSFieldSensTaintAnalysis/Utils/ExtendedValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"

#include "llvm/IR/Instruction.h"

namespace psr {

class FlowFunctionBase : public FlowFunction<ExtendedValue> {
public:
  FlowFunctionBase(const llvm::Instruction *CurrentInst, TraceStats &TStats,
                   const ExtendedValue &ZeroValue)
      : CurrentInst(CurrentInst), TStats(TStats), ZeroValue(ZeroValue) {}
  ~FlowFunctionBase() override = default;

  std::set<ExtendedValue> computeTargets(ExtendedValue Fact) override;
  virtual std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) = 0;

protected:
  const llvm::Instruction *CurrentInst;
  TraceStats &TStats;
  ExtendedValue ZeroValue;
};

} // namespace psr

#endif
