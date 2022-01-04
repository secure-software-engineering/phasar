/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_CHECKOPERANDSFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_CHECKOPERANDSFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class CheckOperandsFlowFunction : public FlowFunctionBase {
public:
  CheckOperandsFlowFunction(const llvm::Instruction *CurrentInst,
                            TraceStats &TStats, const ExtendedValue &ZeroValue)
      : FlowFunctionBase(CurrentInst, TStats, ZeroValue) {}
  ~CheckOperandsFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif
