/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#ifndef MEMSETINSTFLOWFUNCTION_H
#define MEMSETINSTFLOWFUNCTION_H

#include "FlowFunctionBase.h"

namespace psr {

class MemSetInstFlowFunction :
    public FlowFunctionBase
{
public:
  MemSetInstFlowFunction(const llvm::Instruction* _currentInst,
                         TraceStats& _traceStats,
                         ExtendedValue _zeroValue) :
    FlowFunctionBase(_currentInst, _traceStats, _zeroValue) { }
  ~MemSetInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue& fact) override;
};

} // namespace

#endif // MEMSETINSTFLOWFUNCTION_H
