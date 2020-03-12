/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef IDENTITYFLOWFUNCTION_H
#define IDENTITYFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class IdentityFlowFunction : public FlowFunctionBase {
public:
  IdentityFlowFunction(const llvm::Instruction *_currentInst,
                       TraceStats &_traceStats, ExtendedValue _zeroValue)
      : FlowFunctionBase(_currentInst, _traceStats, _zeroValue) {}
  ~IdentityFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &fact) override;
};

} // namespace psr

#endif // IDENTITYFLOWFUNCTION_H
