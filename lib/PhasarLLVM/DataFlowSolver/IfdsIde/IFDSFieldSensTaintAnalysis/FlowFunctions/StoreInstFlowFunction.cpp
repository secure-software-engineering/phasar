/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/StoreInstFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
StoreInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto storeInst = llvm::cast<llvm::StoreInst>(currentInst);

  const auto srcMemLocationMatr = storeInst->getValueOperand();
  const auto dstMemLocationMatr = storeInst->getPointerOperand();

  const auto factMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact);
  auto srcMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(srcMemLocationMatr);
  auto dstMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(dstMemLocationMatr);

  bool isArgumentPatch =
      DataFlowUtils::isPatchableArgumentStore(srcMemLocationMatr, Fact);
  bool isVaListArgumentPatch =
      DataFlowUtils::isPatchableVaListArgument(srcMemLocationMatr, Fact);

  bool isReturnValuePatch =
      DataFlowUtils::isPatchableReturnValue(srcMemLocationMatr, Fact);

  bool isSrcMemLocation = !srcMemLocationSeq.empty();

  std::set<ExtendedValue> targetFacts;

  /*
   * Patch argument
   *
   * We have 3 differenct cases to consider here:
   *
   * 1) Patching of memory location sequence for a regular argument
   * 2) Patching of memory location sequence for a vararg (int foo(int n, ...))
   * 3) Patching of va list memory location sequence for a vararg (int
   * foo(va_list args))
   *
   */
  if (isArgumentPatch) {
    bool patchMemLocation = !dstMemLocationSeq.empty();
    if (patchMemLocation) {
      bool isArgCoerced =
          srcMemLocationMatr->getName().contains_lower("coerce");
      if (isArgCoerced) {
        assert(dstMemLocationSeq.size() > 1);
        dstMemLocationSeq.pop_back();
      }

      const auto patchableMemLocationSeq =
          isVaListArgumentPatch
              ? DataFlowUtils::getVaListMemoryLocationSeqFromFact(Fact)
              : DataFlowUtils::getMemoryLocationSeqFromFact(Fact);

      const auto patchedMemLocationSeq =
          DataFlowUtils::patchMemoryLocationFrame(patchableMemLocationSeq,
                                                  dstMemLocationSeq);

      ExtendedValue ev(Fact);

      if (isVaListArgumentPatch) {
        ev.setVaListMemLocationSeq(patchedMemLocationSeq);
      } else {
        ev.setMemLocationSeq(patchedMemLocationSeq);
        ev.resetVarArgIndex();
      }

      targetFacts.insert(ev);
      traceStats.add(storeInst, dstMemLocationSeq);

      LOG_DEBUG("Patched memory location (arg/store)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(ev);
    }
  }
  /*
   * Patch return value
   */
  else if (isReturnValuePatch) {
    bool patchMemLocation = !dstMemLocationSeq.empty();
    if (patchMemLocation) {
      bool isExtractValue =
          llvm::isa<llvm::ExtractValueInst>(srcMemLocationMatr);
      if (isExtractValue) {
        assert(dstMemLocationSeq.size() > 1);
        dstMemLocationSeq.pop_back();
      }

      const auto patchedMemLocationSeq =
          DataFlowUtils::patchMemoryLocationFrame(factMemLocationSeq,
                                                  dstMemLocationSeq);

      ExtendedValue ev(Fact);
      ev.setMemLocationSeq(patchedMemLocationSeq);

      targetFacts.insert(ev);
      traceStats.add(storeInst, dstMemLocationSeq);

      LOG_DEBUG("Patched memory location (ret/store)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(ev);
    }
  }
  /*
   * If we got a memory location then we need to find all tainted memory
   * locations for it and create a new relocated address that relatively works
   * from the memory location destination. If the value is a pointer so is the
   * desination as the store instruction is defined as <store, ty val, *ty dst>
   * that means we need to remove all facts that started at the destination.
   */
  else if (isSrcMemLocation) {
    bool isArrayDecay = DataFlowUtils::isArrayDecay(srcMemLocationMatr);
    if (isArrayDecay)
      srcMemLocationSeq.pop_back();

    bool genFact = DataFlowUtils::isSubsetMemoryLocationSeq(srcMemLocationSeq,
                                                            factMemLocationSeq);
    bool killFact = DataFlowUtils::isSubsetMemoryLocationSeq(
                        dstMemLocationSeq, factMemLocationSeq) ||
                    DataFlowUtils::isKillAfterStoreFact(Fact);

    if (genFact) {
      const auto relocatableMemLocationSeq =
          DataFlowUtils::getRelocatableMemoryLocationSeq(factMemLocationSeq,
                                                         srcMemLocationSeq);
      const auto relocatedMemLocationSeq =
          DataFlowUtils::joinMemoryLocationSeqs(dstMemLocationSeq,
                                                relocatableMemLocationSeq);

      ExtendedValue ev(Fact);
      ev.setMemLocationSeq(relocatedMemLocationSeq);

      targetFacts.insert(ev);
      traceStats.add(storeInst, dstMemLocationSeq);

      LOG_DEBUG("Relocated memory location (store)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(ev);
    }
    if (!killFact)
      targetFacts.insert(Fact);
  } else {
    bool genFact = DataFlowUtils::isValueTainted(srcMemLocationMatr, Fact);
    bool killFact = DataFlowUtils::isSubsetMemoryLocationSeq(
                        dstMemLocationSeq, factMemLocationSeq) ||
                    DataFlowUtils::isKillAfterStoreFact(Fact);

    if (genFact) {
      ExtendedValue ev(storeInst);
      ev.setMemLocationSeq(dstMemLocationSeq);

      targetFacts.insert(ev);
      traceStats.add(storeInst, dstMemLocationSeq);
    }
    if (!killFact)
      targetFacts.insert(Fact);
  }

  return targetFacts;
}

} // namespace psr
