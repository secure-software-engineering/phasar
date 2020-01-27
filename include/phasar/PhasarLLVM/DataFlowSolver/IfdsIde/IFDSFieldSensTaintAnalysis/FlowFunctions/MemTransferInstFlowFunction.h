/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef MEMTRANSFERINSTFLOWFUNCTION_H
#define MEMTRANSFERINSTFLOWFUNCTION_H

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h>

namespace psr {

class MemTransferInstFlowFunction : public FlowFunctionBase {
public:
  MemTransferInstFlowFunction(const llvm::Instruction *_currentInst,
                              TraceStats &_traceStats, ExtendedValue _zeroValue)
      : FlowFunctionBase(_currentInst, _traceStats, _zeroValue) {}
  ~MemTransferInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &fact) override;
};

} // namespace psr

#endif // MEMTRANSFERINSTFLOWFUNCTION_H
