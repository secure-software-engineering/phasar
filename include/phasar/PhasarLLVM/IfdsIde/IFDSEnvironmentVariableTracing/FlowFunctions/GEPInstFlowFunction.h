/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#ifndef GEPINSTFLOWFUNCTION_H
#define GEPINSTFLOWFUNCTION_H

#include "FlowFunctionBase.h"

namespace psr {

class GEPInstFlowFunction :
    public FlowFunctionBase
{
public:
  GEPInstFlowFunction(const llvm::Instruction* _currentInst,
                      TraceStats& _traceStats,
                      ExtendedValue _zeroValue) :
    FlowFunctionBase(_currentInst, _traceStats, _zeroValue) { }
  ~GEPInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue& fact) override;
};

} // namespace

#endif // GEPINSTFLOWFUNCTION_H
