/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_BRANCHSWITCHINSTFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_BRANCHSWITCHINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class BranchSwitchInstFlowFunction : public FlowFunctionBase {
public:
  BranchSwitchInstFlowFunction(const llvm::Instruction *CurrentInst,
                               TraceStats &TStats,
                               const ExtendedValue &ZeroValue)
      : FlowFunctionBase(CurrentInst, TStats, ZeroValue) {}
  ~BranchSwitchInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif
