/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemTransferInstFlowFunction.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
MemTransferInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto memTransferInst =
      llvm::cast<const llvm::MemTransferInst>(currentInst);

  const auto srcMemLocationMatr = memTransferInst->getRawSource();
  const auto dstMemLocationMatr = memTransferInst->getRawDest();

  const auto factMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact);
  auto srcMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(srcMemLocationMatr);
  auto dstMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(dstMemLocationMatr);

  bool isArgumentPatch = DataFlowUtils::isPatchableArgumentMemcpy(
      memTransferInst->getRawSource(), srcMemLocationSeq, Fact);
  std::set<ExtendedValue> targetFacts;

  /*
   * Patch argument
   */
  if (isArgumentPatch) {
    const auto patchedMemLocationSeq = DataFlowUtils::patchMemoryLocationFrame(
        factMemLocationSeq, dstMemLocationSeq);
    ExtendedValue ev(Fact);
    ev.setMemLocationSeq(patchedMemLocationSeq);
    ev.resetVarArgIndex();

    targetFacts.insert(ev);
    traceStats.add(memTransferInst, dstMemLocationSeq);

    LOG_DEBUG("Patched memory location (arg/memcpy)");
    LOG_DEBUG("Source");
    DataFlowUtils::dumpFact(Fact);
    LOG_DEBUG("Destination");
    DataFlowUtils::dumpFact(ev);
  } else {
    bool isSrcArrayDecay = DataFlowUtils::isArrayDecay(srcMemLocationMatr);
    if (isSrcArrayDecay)
      srcMemLocationSeq.pop_back();

    bool isDstArrayDecay = DataFlowUtils::isArrayDecay(dstMemLocationMatr);
    if (isDstArrayDecay)
      dstMemLocationSeq.pop_back();

    bool genFact = DataFlowUtils::isSubsetMemoryLocationSeq(srcMemLocationSeq,
                                                            factMemLocationSeq);
    bool killFact = DataFlowUtils::isSubsetMemoryLocationSeq(
        dstMemLocationSeq, factMemLocationSeq);

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
      traceStats.add(memTransferInst, dstMemLocationSeq);

      LOG_DEBUG("Relocated memory location (memcpy/memmove)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(ev);
    }
    if (!killFact)
      targetFacts.insert(Fact);
  }

  return targetFacts;
}

} // namespace psr
