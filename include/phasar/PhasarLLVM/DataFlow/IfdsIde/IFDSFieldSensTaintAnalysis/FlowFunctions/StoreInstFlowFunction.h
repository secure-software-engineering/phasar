/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_STOREINSTFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_FLOWFUNCTIONS_STOREINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class StoreInstFlowFunction : public FlowFunctionBase {
public:
  StoreInstFlowFunction(const llvm::Instruction *CurrentInst,
                        TraceStats &TraceStats, const ExtendedValue &ZV)
      : FlowFunctionBase(CurrentInst, TraceStats, ZV) {}
  ~StoreInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif
