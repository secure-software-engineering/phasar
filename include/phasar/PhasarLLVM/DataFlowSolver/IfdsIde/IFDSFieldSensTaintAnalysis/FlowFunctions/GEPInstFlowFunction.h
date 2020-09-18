/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef GEPINSTFLOWFUNCTION_H
#define GEPINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class GEPInstFlowFunction : public FlowFunctionBase {
public:
  GEPInstFlowFunction(const llvm::Instruction *_currentInst,
                      TraceStats &_traceStats, ExtendedValue _zeroValue)
      : FlowFunctionBase(_currentInst, _traceStats, _zeroValue) {}
  ~GEPInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &fact) override;
};

} // namespace psr

#endif // GEPINSTFLOWFUNCTION_H
