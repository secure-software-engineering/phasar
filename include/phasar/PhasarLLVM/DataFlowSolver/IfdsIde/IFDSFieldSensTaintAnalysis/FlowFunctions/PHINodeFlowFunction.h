/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHINODELOWFUNCTION_H
#define PHINODELOWFUNCTION_H

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h>

namespace psr {

class PHINodeFlowFunction : public FlowFunctionBase {
public:
  PHINodeFlowFunction(const llvm::Instruction *_currentInst,
                      TraceStats &_traceStats, ExtendedValue _zeroValue)
      : FlowFunctionBase(_currentInst, _traceStats, _zeroValue) {}
  ~PHINodeFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &fact) override;
};

} // namespace psr

#endif // PHINODELOWFUNCTION_H
