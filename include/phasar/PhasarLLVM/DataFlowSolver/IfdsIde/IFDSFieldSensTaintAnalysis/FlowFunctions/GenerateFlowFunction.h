/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef GENERATEFLOWFUNCTION_H
#define GENERATEFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class GenerateFlowFunction : public FlowFunctionBase {
public:
  GenerateFlowFunction(const llvm::Instruction *CurrentInst,
                       TraceStats &TraceStats, const ExtendedValue &ZV)
      : FlowFunctionBase(CurrentInst, TraceStats, ZV) {}
  ~GenerateFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif // GENERATEFLOWFUNCTION_H
