/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemSetInstFlowFunction.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
MemSetInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto memSetInst = llvm::cast<const llvm::MemSetInst>(currentInst);
  const auto dstMemLocationMatr = memSetInst->getRawDest();

  bool killFact =
      DataFlowUtils::isMemoryLocationTainted(dstMemLocationMatr, Fact);
  if (killFact) {
    traceStats.add(memSetInst);

    return {};
  }

  return {Fact};
}

} // namespace psr
