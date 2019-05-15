/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#ifndef STOREINSTFLOWFUNCTION_H
#define STOREINSTFLOWFUNCTION_H

#include "FlowFunctionBase.h"

namespace psr {

class StoreInstFlowFunction :
    public FlowFunctionBase
{
public:
  StoreInstFlowFunction(const llvm::Instruction* _currentInst,
                        TraceStats& _traceStats,
                        ExtendedValue _zeroValue) :
    FlowFunctionBase(_currentInst, _traceStats, _zeroValue) { }
  ~StoreInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue& fact) override;
};

} // namespace

#endif // STOREINSTFLOWFUNCTION_H
