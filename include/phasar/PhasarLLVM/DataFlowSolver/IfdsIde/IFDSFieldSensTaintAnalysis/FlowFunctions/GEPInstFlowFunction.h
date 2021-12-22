/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef GEPINSTFLOWFUNCTION_H
#define GEPINSTFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class GEPInstFlowFunction : public FlowFunctionBase {
public:
  GEPInstFlowFunction(const llvm::Instruction *CurrentInst, TraceStats &TStats,
                      const ExtendedValue &ZeroValue)
      : FlowFunctionBase(CurrentInst, TStats, ZeroValue) {}
  ~GEPInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif // GEPINSTFLOWFUNCTION_H
