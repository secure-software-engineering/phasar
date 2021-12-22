/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_RETURNINSTFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_RETURNINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class ReturnInstFlowFunction : public FlowFunctionBase {
public:
  ReturnInstFlowFunction(const llvm::Instruction *CurrentInst,
                         TraceStats &TraceStats, const ExtendedValue &ZV)
      : FlowFunctionBase(CurrentInst, TraceStats, ZV) {}
  ~ReturnInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif
