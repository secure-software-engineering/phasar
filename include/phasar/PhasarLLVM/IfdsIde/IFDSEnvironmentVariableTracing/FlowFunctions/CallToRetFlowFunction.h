/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#ifndef CALLTORETFLOWFUNCTION_H
#define CALLTORETFLOWFUNCTION_H

#include "FlowFunctionBase.h"

namespace psr {

class CallToRetFlowFunction :
    public FlowFunctionBase
{
public:
  CallToRetFlowFunction(const llvm::Instruction* _currentInst,
                        TraceStats& _traceStats,
                        ExtendedValue _zeroValue) :
    FlowFunctionBase(_currentInst, _traceStats, _zeroValue) { }
  ~CallToRetFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue& fact) override;
};

} // namespace

#endif // CALLTORETFLOWFUNCTION_H
