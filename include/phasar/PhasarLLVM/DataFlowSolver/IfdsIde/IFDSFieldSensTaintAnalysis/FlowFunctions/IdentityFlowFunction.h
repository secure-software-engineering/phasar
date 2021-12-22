/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef IDENTITYFLOWFUNCTION_H
#define IDENTITYFLOWFUNCTION_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

namespace psr {

class IdentityFlowFunction : public FlowFunctionBase {
public:
  IdentityFlowFunction(const llvm::Instruction *CurrentInst,
                       TraceStats &TraceStats, const ExtendedValue &ZV)
      : FlowFunctionBase(CurrentInst, TraceStats, ZV) {}
  ~IdentityFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &Fact) override;
};

} // namespace psr

#endif // IDENTITYFLOWFUNCTION_H
