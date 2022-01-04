/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemSetInstFlowFunction.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
MemSetInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto *const MemSetInst =
      llvm::cast<const llvm::MemSetInst>(CurrentInst);
  auto *const DstMemLocationMatr = MemSetInst->getRawDest();

  bool KillFact =
      DataFlowUtils::isMemoryLocationTainted(DstMemLocationMatr, Fact);
  if (KillFact) {
    TStats.add(MemSetInst);

    return {};
  }

  return {Fact};
}

} // namespace psr
