/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCallee.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include <algorithm>
#include <tuple>

#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

std::set<ExtendedValue>
MapTaintedValuesToCallee::computeTargets(ExtendedValue Fact) {
  bool isFactVarArgTemplate = Fact.isVarArgTemplate();
  if (isFactVarArgTemplate)
    return {};

  std::set<ExtendedValue> targetGlobalFacts;
  std::set<ExtendedValue> targetParamFacts;

  bool isGlobalMemLocationFact = DataFlowUtils::isGlobalMemoryLocationSeq(
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact));
  if (isGlobalMemLocationFact)
    targetGlobalFacts.insert(Fact);

  long varArgIndex = 0L;

  const auto sanitizedArgList = DataFlowUtils::getSanitizedArgList(
      callInst, destFun, zeroValue.getValue());

  for (const auto &argParamTriple : sanitizedArgList) {

    const auto arg = std::get<0>(argParamTriple);
    const auto argMemLocationSeq = std::get<1>(argParamTriple);
    const auto param = std::get<2>(argParamTriple);

    bool isVarArgParam =
        DataFlowUtils::isVarArgParam(param, zeroValue.getValue());
    bool isVarArgFact = Fact.isVarArg();

    bool isArgMemLocation = !argMemLocationSeq.empty();
    if (isArgMemLocation) {

      const auto factMemLocationSeq =
          isVarArgFact ? DataFlowUtils::getVaListMemoryLocationSeqFromFact(Fact)
                       : DataFlowUtils::getMemoryLocationSeqFromFact(Fact);

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

        ExtendedValue ev(Fact);
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
        DataFlowUtils::dumpFact(Fact);
        LOG_DEBUG("Destination");
        DataFlowUtils::dumpFact(ev);
      }
    } else {
      bool genFact = DataFlowUtils::isValueTainted(arg, Fact);
      if (genFact) {
        std::vector<const llvm::Value *> patchablePart{param};

        ExtendedValue ev(Fact);
        ev.setMemLocationSeq(patchablePart);
        if (isVarArgParam)
          ev.setVarArgIndex(varArgIndex);

        targetParamFacts.insert(ev);

        LOG_DEBUG("Added patchable memory location (caller -> callee)");
        LOG_DEBUG("Source");
        DataFlowUtils::dumpFact(Fact);
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
