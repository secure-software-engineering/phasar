/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/BranchSwitchInstFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
BranchSwitchInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const llvm::Value *Condition = nullptr;

  if (const auto *const BranchInst =
          llvm::dyn_cast<llvm::BranchInst>(currentInst)) {
    bool IsConditional = BranchInst->isConditional();

    if (IsConditional) {
      Condition = BranchInst->getCondition();
    }
  } else if (const auto *const SwitchInst =
                 llvm::dyn_cast<llvm::SwitchInst>(currentInst)) {
    Condition = SwitchInst->getCondition();
  } else {
    assert(false && "This MUST not happen");
  }

  if (Condition) {
    bool IsConditionTainted =
        DataFlowUtils::isValueTainted(Condition, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(Condition, Fact);

    if (IsConditionTainted) {
      const auto *const StartBasicBlock = currentInst->getParent();
      const auto StartBasicBlockLabel = StartBasicBlock->getName();

      LOG_DEBUG("Searching end of block label for: " << StartBasicBlockLabel);

      const auto *const EndBasicBlock =
          DataFlowUtils::getEndOfTaintedBlock(StartBasicBlock);
      const auto EndBasicBlockLabel =
          EndBasicBlock ? EndBasicBlock->getName() : "";

      LOG_DEBUG("End of block label: " << EndBasicBlockLabel);

      ExtendedValue EV(currentInst);
      EV.setEndOfTaintedBlockLabel(EndBasicBlockLabel);

      traceStats.add(currentInst);

      return {Fact, EV};
    }
  }

  return {Fact};
}

} // namespace psr
