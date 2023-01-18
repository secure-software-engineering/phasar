/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/BranchSwitchInstFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include "llvm/IR/Instructions.h"

namespace psr {

std::set<ExtendedValue>
BranchSwitchInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const llvm::Value *Condition = nullptr;

  if (const auto *const BranchInst =
          llvm::dyn_cast<llvm::BranchInst>(CurrentInst)) {
    bool IsConditional = BranchInst->isConditional();

    if (IsConditional) {
      Condition = BranchInst->getCondition();
    }
  } else if (const auto *const SwitchInst =
                 llvm::dyn_cast<llvm::SwitchInst>(CurrentInst)) {
    Condition = SwitchInst->getCondition();
  } else {
    assert(false && "This MUST not happen");
  }

  if (Condition) {
    bool IsConditionTainted =
        DataFlowUtils::isValueTainted(Condition, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(Condition, Fact);

    if (IsConditionTainted) {
      const auto *const StartBasicBlock = CurrentInst->getParent();
      const auto StartBasicBlockLabel = StartBasicBlock->getName();

      LOG_DEBUG("Searching end of block label for: " << StartBasicBlockLabel);

      const auto *const EndBasicBlock =
          DataFlowUtils::getEndOfTaintedBlock(StartBasicBlock);
      const auto EndBasicBlockLabel =
          EndBasicBlock ? EndBasicBlock->getName().str() : "";

      LOG_DEBUG("End of block label: " << EndBasicBlockLabel);

      ExtendedValue EV(CurrentInst);
      EV.setEndOfTaintedBlockLabel(EndBasicBlockLabel);

      TStats.add(CurrentInst);

      return {Fact, EV};
    }
  }

  return {Fact};
}

} // namespace psr
