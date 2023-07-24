/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/StoreInstFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

namespace psr {

std::set<ExtendedValue>
StoreInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto *const StoreInst = llvm::cast<llvm::StoreInst>(CurrentInst);

  const auto *const SrcMemLocationMatr = StoreInst->getValueOperand();
  const auto *const DstMemLocationMatr = StoreInst->getPointerOperand();

  const auto FactMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact);
  auto SrcMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(SrcMemLocationMatr);
  auto DstMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(DstMemLocationMatr);

  bool IsArgumentPatch =
      DataFlowUtils::isPatchableArgumentStore(SrcMemLocationMatr, Fact);
  bool IsVaListArgumentPatch =
      DataFlowUtils::isPatchableVaListArgument(SrcMemLocationMatr, Fact);

  bool IsReturnValuePatch =
      DataFlowUtils::isPatchableReturnValue(SrcMemLocationMatr, Fact);

  bool IsSrcMemLocation = !SrcMemLocationSeq.empty();

  std::set<ExtendedValue> TargetFacts;

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
  if (IsArgumentPatch) {
    bool PatchMemLocation = !DstMemLocationSeq.empty();
    if (PatchMemLocation) {
      bool IsArgCoerced = SrcMemLocationMatr->getName().contains("coerce");
      if (IsArgCoerced) {
        assert(DstMemLocationSeq.size() > 1);
        DstMemLocationSeq.pop_back();
      }

      const auto PatchableMemLocationSeq =
          IsVaListArgumentPatch
              ? DataFlowUtils::getVaListMemoryLocationSeqFromFact(Fact)
              : DataFlowUtils::getMemoryLocationSeqFromFact(Fact);

      const auto PatchedMemLocationSeq =
          DataFlowUtils::patchMemoryLocationFrame(PatchableMemLocationSeq,
                                                  DstMemLocationSeq);

      ExtendedValue EV(Fact);

      if (IsVaListArgumentPatch) {
        EV.setVaListMemLocationSeq(PatchedMemLocationSeq);
      } else {
        EV.setMemLocationSeq(PatchedMemLocationSeq);
        EV.resetVarArgIndex();
      }

      TargetFacts.insert(EV);
      TStats.add(StoreInst, DstMemLocationSeq);

      LOG_DEBUG("Patched memory location (arg/store)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(EV);
    }
  }
  /*
   * Patch return value
   */
  else if (IsReturnValuePatch) {
    bool PatchMemLocation = !DstMemLocationSeq.empty();
    if (PatchMemLocation) {
      bool IsExtractValue =
          llvm::isa<llvm::ExtractValueInst>(SrcMemLocationMatr);
      if (IsExtractValue) {
        assert(DstMemLocationSeq.size() > 1);
        DstMemLocationSeq.pop_back();
      }

      const auto PatchedMemLocationSeq =
          DataFlowUtils::patchMemoryLocationFrame(FactMemLocationSeq,
                                                  DstMemLocationSeq);

      ExtendedValue EV(Fact);
      EV.setMemLocationSeq(PatchedMemLocationSeq);

      TargetFacts.insert(EV);
      TStats.add(StoreInst, DstMemLocationSeq);

      LOG_DEBUG("Patched memory location (ret/store)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(EV);
    }
  }
  /*
   * If we got a memory location then we need to find all tainted memory
   * locations for it and create a new relocated address that relatively works
   * from the memory location destination. If the value is a pointer so is the
   * desination as the store instruction is defined as <store, ty val, *ty dst>
   * that means we need to remove all facts that started at the destination.
   */
  else if (IsSrcMemLocation) {
    bool IsArrayDecay = DataFlowUtils::isArrayDecay(SrcMemLocationMatr);
    if (IsArrayDecay) {
      SrcMemLocationSeq.pop_back();
    }

    bool GenFact = DataFlowUtils::isSubsetMemoryLocationSeq(SrcMemLocationSeq,
                                                            FactMemLocationSeq);
    bool KillFact = DataFlowUtils::isSubsetMemoryLocationSeq(
                        DstMemLocationSeq, FactMemLocationSeq) ||
                    DataFlowUtils::isKillAfterStoreFact(Fact);

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
      TStats.add(StoreInst, DstMemLocationSeq);

      LOG_DEBUG("Relocated memory location (store)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(EV);
    }
    if (!KillFact) {
      TargetFacts.insert(Fact);
    }
  } else {
    bool GenFact = DataFlowUtils::isValueTainted(SrcMemLocationMatr, Fact);
    bool KillFact = DataFlowUtils::isSubsetMemoryLocationSeq(
                        DstMemLocationSeq, FactMemLocationSeq) ||
                    DataFlowUtils::isKillAfterStoreFact(Fact);

    if (GenFact) {
      ExtendedValue EV(StoreInst);
      EV.setMemLocationSeq(DstMemLocationSeq);

      TargetFacts.insert(EV);
      TStats.add(StoreInst, DstMemLocationSeq);
    }
    if (!KillFact) {
      TargetFacts.insert(Fact);
    }
  }

  return TargetFacts;
}

} // namespace psr
