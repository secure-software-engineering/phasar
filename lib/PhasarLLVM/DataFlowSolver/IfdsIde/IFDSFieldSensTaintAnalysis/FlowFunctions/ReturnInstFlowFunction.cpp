/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/ReturnInstFlowFunction.h>

namespace psr {

std::set<ExtendedValue>
ReturnInstFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  const auto retInst = llvm::cast<llvm::ReturnInst>(currentInst);
  const auto retValMemLocationMatr = retInst->getReturnValue();

  if (retValMemLocationMatr) {
    bool isRetValTainted =
        DataFlowUtils::isValueTainted(retValMemLocationMatr, fact) ||
        DataFlowUtils::isMemoryLocationTainted(retValMemLocationMatr, fact);

    /*
     * We don't need to GEN/KILL any facts here as this is all handled
     * in the map to callee flow function. Whole purpose of this flow
     * function is to make sure that a tainted return statement of an
     * entry point is added as for that case no mapping function is called.
     */
    if (isRetValTainted)
      traceStats.add(retInst);
  }

  return {fact};
}

} // namespace psr
