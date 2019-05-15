/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#ifndef BRANCHSWITCHINSTFLOWFUNCTION_H
#define BRANCHSWITCHINSTFLOWFUNCTION_H

#include "FlowFunctionBase.h"

namespace psr {

class BranchSwitchInstFlowFunction :
    public FlowFunctionBase
{
public:
  BranchSwitchInstFlowFunction(const llvm::Instruction* _currentInst,
                               TraceStats& _traceStats,
                               ExtendedValue _zeroValue) :
    FlowFunctionBase(_currentInst, _traceStats, _zeroValue) { }
  ~BranchSwitchInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue& fact) override;
};

} // namespace

#endif // BRANCHSWITCHINSTFLOWFUNCTION_H
