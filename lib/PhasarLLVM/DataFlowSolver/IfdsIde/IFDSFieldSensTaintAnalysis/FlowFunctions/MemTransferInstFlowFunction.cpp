/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemTransferInstFlowFunction.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
MemTransferInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto *const MemTransferInst =
      llvm::cast<const llvm::MemTransferInst>(CurrentInst);

  auto *const SrcMemLocationMatr = MemTransferInst->getRawSource();
  auto *const DstMemLocationMatr = MemTransferInst->getRawDest();

  const auto FactMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact);
  auto SrcMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(SrcMemLocationMatr);
  auto DstMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(DstMemLocationMatr);

  bool IsArgumentPatch = DataFlowUtils::isPatchableArgumentMemcpy(
      MemTransferInst->getRawSource(), SrcMemLocationSeq, Fact);
  std::set<ExtendedValue> TargetFacts;

  /*
   * Patch argument
   */
  if (IsArgumentPatch) {
    const auto PatchedMemLocationSeq = DataFlowUtils::patchMemoryLocationFrame(
        FactMemLocationSeq, DstMemLocationSeq);
    ExtendedValue EV(Fact);
    EV.setMemLocationSeq(PatchedMemLocationSeq);
    EV.resetVarArgIndex();

    TargetFacts.insert(EV);
    TStats.add(MemTransferInst, DstMemLocationSeq);

    LOG_DEBUG("Patched memory location (arg/memcpy)");
    LOG_DEBUG("Source");
    DataFlowUtils::dumpFact(Fact);
    LOG_DEBUG("Destination");
    DataFlowUtils::dumpFact(EV);
  } else {
    bool IsSrcArrayDecay = DataFlowUtils::isArrayDecay(SrcMemLocationMatr);
    if (IsSrcArrayDecay) {
      SrcMemLocationSeq.pop_back();
    }

    bool IsDstArrayDecay = DataFlowUtils::isArrayDecay(DstMemLocationMatr);
    if (IsDstArrayDecay) {
      DstMemLocationSeq.pop_back();
    }

    bool GenFact = DataFlowUtils::isSubsetMemoryLocationSeq(SrcMemLocationSeq,
                                                            FactMemLocationSeq);
    bool KillFact = DataFlowUtils::isSubsetMemoryLocationSeq(
        DstMemLocationSeq, FactMemLocationSeq);

    if (GenFact) {
      const auto RelocatableMemLocationSeq =
          DataFlowUtils::getRelocatableMemoryLocationSeq(FactMemLocationSeq,
                                                         SrcMemLocationSeq);
      const auto RelocatedMemLocationSeq =
          DataFlowUtils::joinMemoryLocationSeqs(DstMemLocationSeq,
                                                RelocatableMemLocationSeq);
      ExtendedValue EV(Fact);
      EV.setMemLocationSeq(RelocatedMemLocationSeq);

      TargetFacts.insert(EV);
      TStats.add(MemTransferInst, DstMemLocationSeq);

      LOG_DEBUG("Relocated memory location (memcpy/memmove)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(EV);
    }
    if (!KillFact) {
      TargetFacts.insert(Fact);
    }
  }

  return TargetFacts;
}

} // namespace psr
