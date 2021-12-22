/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_VAENDINSTFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_VAENDINSTFLOWFUNCTION_H

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

#endif
