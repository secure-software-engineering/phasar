/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#ifndef RETURNINSTFLOWFUNCTION_H
#define RETURNINSTFLOWFUNCTION_H

#include "FlowFunctionBase.h"

namespace psr {

class ReturnInstFlowFunction :
    public FlowFunctionBase
{
public:
  ReturnInstFlowFunction(const llvm::Instruction* _currentInst,
                         TraceStats& _traceStats,
                         ExtendedValue _zeroValue) :
    FlowFunctionBase(_currentInst, _traceStats, _zeroValue) { }
  ~ReturnInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue& fact) override;
};

} // namespace

#endif // RETURNINSTFLOWFUNCTION_H
