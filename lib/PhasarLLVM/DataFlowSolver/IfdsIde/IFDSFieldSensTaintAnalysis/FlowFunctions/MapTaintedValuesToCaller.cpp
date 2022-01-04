/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCaller.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include <algorithm>

#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

std::set<ExtendedValue>
MapTaintedValuesToCaller::computeTargets(ExtendedValue Fact) {
  std::set<ExtendedValue> TargetGlobalFacts;
  std::set<ExtendedValue> TargetRetFacts;

  bool IsGlobalMemLocationFact = DataFlowUtils::isGlobalMemoryLocationSeq(
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact));
  if (IsGlobalMemLocationFact) {
    TargetGlobalFacts.insert(Fact);
  }

  auto *const RetValMemLocationMatr = RetInst->getReturnValue();
  if (!RetValMemLocationMatr) {
    return TargetGlobalFacts;
  }

  auto RetValMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(RetValMemLocationMatr);

  bool IsRetValMemLocation = !RetValMemLocationSeq.empty();
  if (IsRetValMemLocation) {
    const auto FactMemLocationSeq =
        DataFlowUtils::getMemoryLocationSeqFromFact(Fact);

    bool IsArrayDecay = DataFlowUtils::isArrayDecay(RetValMemLocationMatr);
    if (IsArrayDecay) {
      RetValMemLocationSeq.pop_back();
    }

    bool GenFact = DataFlowUtils::isSubsetMemoryLocationSeq(
        RetValMemLocationSeq, FactMemLocationSeq);
    if (GenFact) {
      const auto RelocatableMemLocationSeq =
          DataFlowUtils::getRelocatableMemoryLocationSeq(FactMemLocationSeq,
                                                         RetValMemLocationSeq);
      std::vector<const llvm::Value *> PatchablePart{CallInst};
      const auto PatchableMemLocationSeq =
          DataFlowUtils::joinMemoryLocationSeqs(PatchablePart,
                                                RelocatableMemLocationSeq);

      /*
       * We need to set this to call inst because we can have the case where we
       * only return the call inst in the mem location sequence (which is not a
       * a memory address). We then land in the else branch below and need to
       * find the call instance (see test case 230-function-ptr-2).
       */
      ExtendedValue EV(CallInst);
      EV.setMemLocationSeq(PatchableMemLocationSeq);

      TargetRetFacts.insert(EV);

      LOG_DEBUG("Added patchable memory location (caller <- callee)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(EV);
    }
  } else {
    bool GenFact = DataFlowUtils::isValueTainted(RetValMemLocationMatr, Fact);
    if (GenFact) {
      std::vector<const llvm::Value *> PatchablePart{CallInst};

      ExtendedValue EV(CallInst);
      EV.setMemLocationSeq(PatchablePart);

      TargetRetFacts.insert(EV);

      LOG_DEBUG("Added patchable memory location (caller <- callee)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(Fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(EV);
    }
  }

  bool AddLineNumbers = !TargetRetFacts.empty();
  if (AddLineNumbers) {
    TraceStats.add(CallInst);
  }

  std::set<ExtendedValue> TargetFacts;
  std::set_union(TargetGlobalFacts.begin(), TargetGlobalFacts.end(),
                 TargetRetFacts.begin(), TargetRetFacts.end(),
                 std::inserter(TargetFacts, TargetFacts.begin()));

  return TargetFacts;
}

} // namespace psr
