/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_FLOWFUNCTIONBASE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_FLOWFUNCTIONBASE_H

#include "../Stats/TraceStats.h"

#include "../Utils/DataFlowUtils.h"
#include "../Utils/Log.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/Domain/ExtendedValue.h"

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
