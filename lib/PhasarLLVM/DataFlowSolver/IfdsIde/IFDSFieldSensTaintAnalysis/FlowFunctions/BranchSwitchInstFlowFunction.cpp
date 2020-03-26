/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/BranchSwitchInstFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
BranchSwitchInstFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  const llvm::Value *condition = nullptr;

  if (const auto branchInst = llvm::dyn_cast<llvm::BranchInst>(currentInst)) {
    bool isConditional = branchInst->isConditional();

    if (isConditional)
      condition = branchInst->getCondition();
  } else if (const auto switchInst =
                 llvm::dyn_cast<llvm::SwitchInst>(currentInst)) {
    condition = switchInst->getCondition();
  } else {
    assert(false && "This MUST not happen");
  }

  if (condition) {
    bool isConditionTainted =
        DataFlowUtils::isValueTainted(condition, fact) ||
        DataFlowUtils::isMemoryLocationTainted(condition, fact);

    if (isConditionTainted) {
      const auto startBasicBlock = currentInst->getParent();
      const auto startBasicBlockLabel = startBasicBlock->getName();

      LOG_DEBUG("Searching end of block label for: " << startBasicBlockLabel);

      const auto endBasicBlock =
          DataFlowUtils::getEndOfTaintedBlock(startBasicBlock);
      const auto endBasicBlockLabel =
          endBasicBlock ? endBasicBlock->getName() : "";

      LOG_DEBUG("End of block label: " << endBasicBlockLabel);

      ExtendedValue ev(currentInst);
      ev.setEndOfTaintedBlockLabel(endBasicBlockLabel);

      traceStats.add(currentInst);

      return {fact, ev};
    }
  }

  return {fact};
}

} // namespace psr
