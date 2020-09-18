/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef VAENDINSTFLOWFUNCTION_H
#define VAENDINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class VAEndInstFlowFunction : public FlowFunctionBase {
public:
  VAEndInstFlowFunction(const llvm::Instruction *_currentInst,
                        TraceStats &_traceStats, ExtendedValue _zeroValue)
      : FlowFunctionBase(_currentInst, _traceStats, _zeroValue) {}
  ~VAEndInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &fact) override;
};

} // namespace psr

#endif // VAENDINSTFLOWFUNCTION_H
