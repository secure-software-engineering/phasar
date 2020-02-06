/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCallee.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h>

#include <algorithm>
#include <tuple>

#include <phasar/Utils/LLVMShorthands.h>

namespace psr {

std::set<ExtendedValue>
MapTaintedValuesToCallee::computeTargets(ExtendedValue fact) {
  bool isFactVarArgTemplate = fact.isVarArgTemplate();
  if (isFactVarArgTemplate)
    return {};

  std::set<ExtendedValue> targetGlobalFacts;
  std::set<ExtendedValue> targetParamFacts;

  bool isGlobalMemLocationFact = DataFlowUtils::isGlobalMemoryLocationSeq(
      DataFlowUtils::getMemoryLocationSeqFromFact(fact));
  if (isGlobalMemLocationFact)
    targetGlobalFacts.insert(fact);

  long varArgIndex = 0L;

  const auto sanitizedArgList = DataFlowUtils::getSanitizedArgList(
      callInst, destFun, zeroValue.getValue());

  for (const auto &argParamTriple : sanitizedArgList) {

    const auto arg = std::get<0>(argParamTriple);
    const auto argMemLocationSeq = std::get<1>(argParamTriple);
    const auto param = std::get<2>(argParamTriple);

    bool isVarArgParam =
        DataFlowUtils::isVarArgParam(param, zeroValue.getValue());
    bool isVarArgFact = fact.isVarArg();

    bool isArgMemLocation = !argMemLocationSeq.empty();
    if (isArgMemLocation) {

      const auto factMemLocationSeq =
          isVarArgFact ? DataFlowUtils::getVaListMemoryLocationSeqFromFact(fact)
                       : DataFlowUtils::getMemoryLocationSeqFromFact(fact);

      bool genFact = DataFlowUtils::isSubsetMemoryLocationSeq(
          argMemLocationSeq, factMemLocationSeq);
      if (genFact) {
        const auto relocatableMemLocationSeq =
            DataFlowUtils::getRelocatableMemoryLocationSeq(factMemLocationSeq,
                                                           argMemLocationSeq);
        std::vector<const llvm::Value *> patchablePart{param};
        const auto patchableMemLocationSeq =
            DataFlowUtils::joinMemoryLocationSeqs(patchablePart,
                                                  relocatableMemLocationSeq);

        ExtendedValue ev(fact);
        if (isVarArgFact) {
          ev.setVaListMemLocationSeq(patchableMemLocationSeq);
        } else {
          ev.setMemLocationSeq(patchableMemLocationSeq);
        }

        if (isVarArgParam)
          ev.setVarArgIndex(varArgIndex);

        targetParamFacts.insert(ev);

        LOG_DEBUG("Added patchable memory location (caller -> callee)");
        LOG_DEBUG("Source");
        DataFlowUtils::dumpFact(fact);
        LOG_DEBUG("Destination");
        DataFlowUtils::dumpFact(ev);
      }
    } else {
      bool genFact = DataFlowUtils::isValueTainted(arg, fact);
      if (genFact) {
        std::vector<const llvm::Value *> patchablePart{param};

        ExtendedValue ev(fact);
        ev.setMemLocationSeq(patchablePart);
        if (isVarArgParam)
          ev.setVarArgIndex(varArgIndex);

        targetParamFacts.insert(ev);

        LOG_DEBUG("Added patchable memory location (caller -> callee)");
        LOG_DEBUG("Source");
        DataFlowUtils::dumpFact(fact);
        LOG_DEBUG("Destination");
        DataFlowUtils::dumpFact(ev);
      }
    }

    if (isVarArgParam)
      ++varArgIndex;
  }

  bool addLineNumber = !targetParamFacts.empty();
  if (addLineNumber)
    traceStats.add(callInst);

  std::set<ExtendedValue> targetFacts;
  std::set_union(targetGlobalFacts.begin(), targetGlobalFacts.end(),
                 targetParamFacts.begin(), targetParamFacts.end(),
                 std::inserter(targetFacts, targetFacts.begin()));

  return targetFacts;
}

} // namespace psr
