/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef STOREINSTFLOWFUNCTION_H
#define STOREINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

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

#endif // STOREINSTFLOWFUNCTION_H
