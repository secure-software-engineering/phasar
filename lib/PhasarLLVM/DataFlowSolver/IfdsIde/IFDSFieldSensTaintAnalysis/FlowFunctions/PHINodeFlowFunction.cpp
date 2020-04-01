/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/PHINodeFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
PHINodeFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto phiNodeInst = llvm::cast<llvm::PHINode>(currentInst);

  for (const auto block : phiNodeInst->blocks()) {
    const auto incomingValue = phiNodeInst->getIncomingValueForBlock(block);

    bool isIncomingValueTainted =
        DataFlowUtils::isValueTainted(incomingValue, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(incomingValue, Fact);

    if (isIncomingValueTainted) {
      traceStats.add(phiNodeInst);

      return {Fact, ExtendedValue(phiNodeInst)};
    }
  }

  return {Fact};
}

} // namespace psr
