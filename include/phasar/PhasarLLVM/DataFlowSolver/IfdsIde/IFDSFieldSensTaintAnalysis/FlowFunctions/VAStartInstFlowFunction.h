/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef VASTARTINSTFLOWFUNCTION_H
#define VASTARTINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class VAStartInstFlowFunction : public FlowFunctionBase {
public:
  VAStartInstFlowFunction(const llvm::Instruction *CurrentInst,
                          TraceStats &TraceStats, const ExtendedValue &ZV)
      : FlowFunctionBase(CurrentInst, TraceStats, ZV) {}
  ~VAStartInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif // VASTARTINSTFLOWFUNCTION_H
