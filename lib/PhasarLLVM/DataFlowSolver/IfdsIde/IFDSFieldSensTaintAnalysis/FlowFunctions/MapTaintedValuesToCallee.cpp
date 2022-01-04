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
  bool IsFactVarArgTemplate = Fact.isVarArgTemplate();
  if (IsFactVarArgTemplate) {
    return {};
  }

  std::set<ExtendedValue> TargetGlobalFacts;
  std::set<ExtendedValue> TargetParamFacts;

  bool IsGlobalMemLocationFact = DataFlowUtils::isGlobalMemoryLocationSeq(
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact));
  if (IsGlobalMemLocationFact) {
    TargetGlobalFacts.insert(Fact);
  }

  long VarArgIndex = 0L;

  const auto SanitizedArgList = DataFlowUtils::getSanitizedArgList(
      CallInst, DestFun, ZeroValue.getValue());

  for (const auto &ArgParamTriple : SanitizedArgList) {

    const auto *const Arg = std::get<0>(ArgParamTriple);
    const auto &ArgMemLocationSeq = std::get<1>(ArgParamTriple);
    const auto *const Param = std::get<2>(ArgParamTriple);

    bool IsVarArgParam =
        DataFlowUtils::isVarArgParam(Param, ZeroValue.getValue());
    bool IsVarArgFact = Fact.isVarArg();

    bool IsArgMemLocation = !ArgMemLocationSeq.empty();
    if (IsArgMemLocation) {

      const auto FactMemLocationSeq =
          IsVarArgFact ? DataFlowUtils::getVaListMemoryLocationSeqFromFact(Fact)
                       : DataFlowUtils::getMemoryLocationSeqFromFact(Fact);

      bool GenFact = DataFlowUtils::isSubsetMemoryLocationSeq(
          ArgMemLocationSeq, FactMemLocationSeq);
      if (GenFact) {
        const auto RelocatableMemLocationSeq =
            DataFlowUtils::getRelocatableMemoryLocationSeq(FactMemLocationSeq,
                                                           ArgMemLocationSeq);
        std::vector<const llvm::Value *> PatchablePart{Param};
        const auto PatchableMemLocationSeq =
            DataFlowUtils::joinMemoryLocationSeqs(PatchablePart,
                                                  RelocatableMemLocationSeq);

        ExtendedValue EV(Fact);
        if (IsVarArgFact) {
          EV.setVaListMemLocationSeq(PatchableMemLocationSeq);
        } else {
          EV.setMemLocationSeq(PatchableMemLocationSeq);
        }

        if (IsVarArgParam) {
          EV.setVarArgIndex(VarArgIndex);
        }

        TargetParamFacts.insert(EV);

        LOG_DEBUG("Added patchable memory location (caller -> callee)");
        LOG_DEBUG("Source");
        DataFlowUtils::dumpFact(Fact);
        LOG_DEBUG("Destination");
        DataFlowUtils::dumpFact(EV);
      }
    } else {
      bool GenFact = DataFlowUtils::isValueTainted(Arg, Fact);
      if (GenFact) {
        std::vector<const llvm::Value *> PatchablePart{Param};

        ExtendedValue EV(Fact);
        EV.setMemLocationSeq(PatchablePart);
        if (IsVarArgParam) {
          EV.setVarArgIndex(VarArgIndex);
        }

        TargetParamFacts.insert(EV);

        LOG_DEBUG("Added patchable memory location (caller -> callee)");
        LOG_DEBUG("Source");
        DataFlowUtils::dumpFact(Fact);
        LOG_DEBUG("Destination");
        DataFlowUtils::dumpFact(EV);
      }
    }

    if (IsVarArgParam) {
      ++VarArgIndex;
    }
  }

  bool AddLineNumber = !TargetParamFacts.empty();
  if (AddLineNumber) {
    TStats.add(CallInst);
  }

  std::set<ExtendedValue> TargetFacts;
  std::set_union(TargetGlobalFacts.begin(), TargetGlobalFacts.end(),
                 TargetParamFacts.begin(), TargetParamFacts.end(),
                 std::inserter(TargetFacts, TargetFacts.begin()));

  return TargetFacts;
}

} // namespace psr
