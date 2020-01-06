/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCaller.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h>

#include <algorithm>

#include <phasar/Utils/LLVMShorthands.h>

namespace psr {

std::set<ExtendedValue>
MapTaintedValuesToCaller::computeTargets(ExtendedValue fact) {
  std::set<ExtendedValue> targetGlobalFacts;
  std::set<ExtendedValue> targetRetFacts;

  bool isGlobalMemLocationFact = DataFlowUtils::isGlobalMemoryLocationSeq(
      DataFlowUtils::getMemoryLocationSeqFromFact(fact));
  if (isGlobalMemLocationFact)
    targetGlobalFacts.insert(fact);

  const auto retValMemLocationMatr = retInst->getReturnValue();
  if (!retValMemLocationMatr)
    return targetGlobalFacts;

  auto retValMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(retValMemLocationMatr);

  bool isRetValMemLocation = !retValMemLocationSeq.empty();
  if (isRetValMemLocation) {
    const auto factMemLocationSeq =
        DataFlowUtils::getMemoryLocationSeqFromFact(fact);

    bool isArrayDecay = DataFlowUtils::isArrayDecay(retValMemLocationMatr);
    if (isArrayDecay)
      retValMemLocationSeq.pop_back();

    bool genFact = DataFlowUtils::isSubsetMemoryLocationSeq(
        retValMemLocationSeq, factMemLocationSeq);
    if (genFact) {
      const auto relocatableMemLocationSeq =
          DataFlowUtils::getRelocatableMemoryLocationSeq(factMemLocationSeq,
                                                         retValMemLocationSeq);
      std::vector<const llvm::Value *> patchablePart{callInst};
      const auto patchableMemLocationSeq =
          DataFlowUtils::joinMemoryLocationSeqs(patchablePart,
                                                relocatableMemLocationSeq);

      /*
       * We need to set this to call inst because we can have the case where we
       * only return the call inst in the mem location sequence (which is not a
       * a memory address). We then land in the else branch below and need to
       * find the call instance (see test case 230-function-ptr-2).
       */
      ExtendedValue ev(callInst);
      ev.setMemLocationSeq(patchableMemLocationSeq);

      targetRetFacts.insert(ev);

      LOG_DEBUG("Added patchable memory location (caller <- callee)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(ev);
    }
  } else {
    bool genFact = DataFlowUtils::isValueTainted(retValMemLocationMatr, fact);
    if (genFact) {
      std::vector<const llvm::Value *> patchablePart{callInst};

      ExtendedValue ev(callInst);
      ev.setMemLocationSeq(patchablePart);

      targetRetFacts.insert(ev);

      LOG_DEBUG("Added patchable memory location (caller <- callee)");
      LOG_DEBUG("Source");
      DataFlowUtils::dumpFact(fact);
      LOG_DEBUG("Destination");
      DataFlowUtils::dumpFact(ev);
    }
  }

  bool addLineNumbers = !targetRetFacts.empty();
  if (addLineNumbers)
    traceStats.add(callInst);

  std::set<ExtendedValue> targetFacts;
  std::set_union(targetGlobalFacts.begin(), targetGlobalFacts.end(),
                 targetRetFacts.begin(), targetRetFacts.end(),
                 std::inserter(targetFacts, targetFacts.begin()));

  return targetFacts;
}

} // namespace psr
