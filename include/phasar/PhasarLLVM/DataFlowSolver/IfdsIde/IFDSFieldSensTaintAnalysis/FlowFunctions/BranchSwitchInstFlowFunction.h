/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef BRANCHSWITCHINSTFLOWFUNCTION_H
#define BRANCHSWITCHINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

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

#endif // BRANCHSWITCHINSTFLOWFUNCTION_H
