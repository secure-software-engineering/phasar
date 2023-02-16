/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/PHINodeFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"

#include "llvm/IR/Instructions.h"

namespace psr {

std::set<ExtendedValue>
PHINodeFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto *const PhiNodeInst = llvm::cast<llvm::PHINode>(CurrentInst);

  for (auto *const Block : PhiNodeInst->blocks()) {
    auto *const IncomingValue = PhiNodeInst->getIncomingValueForBlock(Block);

    bool IsIncomingValueTainted =
        DataFlowUtils::isValueTainted(IncomingValue, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(IncomingValue, Fact);

    if (IsIncomingValueTainted) {
      TStats.add(PhiNodeInst);

      return {Fact, ExtendedValue(PhiNodeInst)};
    }
  }

  return {Fact};
}

} // namespace psr
