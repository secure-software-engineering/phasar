/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MEMTRANSFERINSTFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_MEMTRANSFERINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class MemTransferInstFlowFunction : public FlowFunctionBase {
public:
  MemTransferInstFlowFunction(const llvm::Instruction *CurrentInst,
                              TraceStats &TraceStats, const ExtendedValue &ZV)
      : FlowFunctionBase(CurrentInst, TraceStats, ZV) {}
  ~MemTransferInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif
