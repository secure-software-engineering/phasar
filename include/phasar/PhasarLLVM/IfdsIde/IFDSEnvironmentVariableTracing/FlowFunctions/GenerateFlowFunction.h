/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#ifndef GENERATEFLOWFUNCTION_H
#define GENERATEFLOWFUNCTION_H

#include "FlowFunctionBase.h"

namespace psr {

class GenerateFlowFunction :
    public FlowFunctionBase
{
public:
  GenerateFlowFunction(const llvm::Instruction* _currentInst,
                       TraceStats& _traceStats,
                       ExtendedValue _zeroValue) :
    FlowFunctionBase(_currentInst, _traceStats, _zeroValue) { }
  ~GenerateFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue& fact) override;
};

} // namespace

#endif // GENERATEFLOWFUNCTION_H
