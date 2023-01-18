/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/ReturnInstFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"

namespace psr {

std::set<ExtendedValue>
ReturnInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto *const RetInst = llvm::cast<llvm::ReturnInst>(CurrentInst);
  auto *const RetValMemLocationMatr = RetInst->getReturnValue();

  if (RetValMemLocationMatr) {
    bool IsRetValTainted =
        DataFlowUtils::isValueTainted(RetValMemLocationMatr, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(RetValMemLocationMatr, Fact);

    /*
     * We don't need to GEN/KILL any facts here as this is all handled
     * in the map to callee flow function. Whole purpose of this flow
     * function is to make sure that a tainted return statement of an
     * entry point is added as for that case no mapping function is called.
     */
    if (IsRetValTainted) {
      TStats.add(RetInst);
    }
  }

  return {Fact};
}

} // namespace psr
