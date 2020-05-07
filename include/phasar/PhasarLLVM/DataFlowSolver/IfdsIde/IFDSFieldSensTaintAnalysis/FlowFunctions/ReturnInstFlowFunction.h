/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef RETURNINSTFLOWFUNCTION_H
#define RETURNINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class ReturnInstFlowFunction : public FlowFunctionBase {
public:
  ReturnInstFlowFunction(const llvm::Instruction *_currentInst,
                         TraceStats &_traceStats, ExtendedValue _zeroValue)
      : FlowFunctionBase(_currentInst, _traceStats, _zeroValue) {}
  ~ReturnInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &fact) override;
};

} // namespace psr

#endif // RETURNINSTFLOWFUNCTION_H
