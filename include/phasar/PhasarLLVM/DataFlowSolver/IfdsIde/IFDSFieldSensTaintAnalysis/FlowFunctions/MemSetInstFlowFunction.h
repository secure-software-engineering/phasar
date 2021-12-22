/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef MEMSETINSTFLOWFUNCTION_H
#define MEMSETINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class MemSetInstFlowFunction : public FlowFunctionBase {
public:
  MemSetInstFlowFunction(const llvm::Instruction *CurrentInst,
                         TraceStats &TraceStats, const ExtendedValue &ZV)
      : FlowFunctionBase(CurrentInst, TraceStats, ZV) {}
  ~MemSetInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif // MEMSETINSTFLOWFUNCTION_H
