/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef RETURNINSTFLOWFUNCTION_H
#define RETURNINSTFLOWFUNCTION_H

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

#endif // RETURNINSTFLOWFUNCTION_H
