/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/FlowFunctionBase.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue> FlowFunctionBase::computeTargets(ExtendedValue fact) {
  bool isAutoIdentity = DataFlowUtils::isAutoIdentity(currentInst, fact);
  if (isAutoIdentity)
    return {fact};

  bool isBranchOrSwitchFact = llvm::isa<llvm::BranchInst>(fact.getValue()) ||
                              llvm::isa<llvm::SwitchInst>(fact.getValue());

  if (isBranchOrSwitchFact) {
    bool removeTaintedBlockInst =
        DataFlowUtils::removeTaintedBlockInst(fact, currentInst);
    if (removeTaintedBlockInst)
      return {};

    // traceStats.add(currentInst);

    bool isAutoGEN = DataFlowUtils::isAutoGENInTaintedBlock(currentInst);
    if (isAutoGEN) {
      traceStats.add(currentInst);

      return {fact, ExtendedValue(currentInst)};
    }

    std::set<ExtendedValue> targetFacts;
    targetFacts.insert(fact);

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
    if (const auto storeInst = llvm::dyn_cast<llvm::StoreInst>(currentInst)) {
      const auto dstMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(
              storeInst->getPointerOperand());

      ExtendedValue ev(currentInst);
      ev.setMemLocationSeq(dstMemLocationSeq);

      targetFacts.insert(ev);
      traceStats.add(storeInst, dstMemLocationSeq);
    } else if (const auto memTransferInst =
                   llvm::dyn_cast<llvm::MemTransferInst>(currentInst)) {
      const auto dstMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(
              memTransferInst->getRawDest());

      ExtendedValue ev(currentInst);
      ev.setMemLocationSeq(dstMemLocationSeq);

      targetFacts.insert(ev);
      traceStats.add(memTransferInst, dstMemLocationSeq);
    } else if (const auto retInst =
                   llvm::dyn_cast<llvm::ReturnInst>(currentInst)) {
      traceStats.add(retInst);
    }

    return targetFacts;
  }

  return computeTargetsExt(fact);
}

} // namespace psr
