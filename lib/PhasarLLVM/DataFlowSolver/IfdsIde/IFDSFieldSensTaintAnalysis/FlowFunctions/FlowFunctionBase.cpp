/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue> FlowFunctionBase::computeTargets(ExtendedValue Fact) {
  bool IsAutoIdentity = DataFlowUtils::isAutoIdentity(currentInst, Fact);
  if (IsAutoIdentity) {
    return {Fact};
  }

  bool IsBranchOrSwitchFact = llvm::isa<llvm::BranchInst>(Fact.getValue()) ||
                              llvm::isa<llvm::SwitchInst>(Fact.getValue());

  if (IsBranchOrSwitchFact) {
    bool RemoveTaintedBlockInst =
        DataFlowUtils::removeTaintedBlockInst(Fact, currentInst);
    if (RemoveTaintedBlockInst) {
      return {};
    }

    // traceStats.add(currentInst);

    bool IsAutoGEN = DataFlowUtils::isAutoGENInTaintedBlock(currentInst);
    if (IsAutoGEN) {
      traceStats.add(currentInst);

      return {Fact, ExtendedValue(currentInst)};
    }

    std::set<ExtendedValue> TargetFacts;
    TargetFacts.insert(Fact);

    /*
     * We are only intercepting the branch fact here. All other facts will still
     * be evaluated according to the flow function's logic. This means that e.g.
     * every valid memory instruction (i.e. if src is tainted -> memory location
     * is added) will still gen/kill/id facts. Actually those functions do not
     * have any clue that they behave in a tainted block and there is no way to
     * provide this knowledge due to the distributive property of IFDS.
     *
     * The only cases we need to consider here is the addition of store facts
     * that would not be added in the regular case. In particular all cases
     * where the src is not tainted and the store fact is killed.
     *
     * Note that there is no way to relocate memory addresses here as we are
     * dealing with untainted sources. This means that if e.g. we add a struct
     * then all subparts of it are considered tainted. This should be the only
     * spot where such memory locations are generated.
     */
    if (const auto *const StoreInst =
            llvm::dyn_cast<llvm::StoreInst>(currentInst)) {
      const auto DstMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(
              StoreInst->getPointerOperand());

      ExtendedValue EV(currentInst);
      EV.setMemLocationSeq(DstMemLocationSeq);

      TargetFacts.insert(EV);
      traceStats.add(StoreInst, DstMemLocationSeq);
    } else if (const auto *const MemTransferInst =
                   llvm::dyn_cast<llvm::MemTransferInst>(currentInst)) {
      const auto DstMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(
              MemTransferInst->getRawDest());

      ExtendedValue EV(currentInst);
      EV.setMemLocationSeq(DstMemLocationSeq);

      TargetFacts.insert(EV);
      traceStats.add(MemTransferInst, DstMemLocationSeq);
    } else if (const auto *const RetInst =
                   llvm::dyn_cast<llvm::ReturnInst>(currentInst)) {
      traceStats.add(RetInst);
    }

    return TargetFacts;
  }

  return computeTargetsExt(Fact);
}

} // namespace psr
