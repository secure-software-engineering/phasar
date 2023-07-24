/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAStartInstFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
VAStartInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  std::set<ExtendedValue> TargetFacts;
  TargetFacts.insert(Fact);

  bool IsVarArgTemplateFact = Fact.isVarArgTemplate();
  if (!IsVarArgTemplateFact) {
    return TargetFacts;
  }

  const auto *const VaStartInst = llvm::cast<llvm::VAStartInst>(CurrentInst);
  auto *const VaListMemLocationMatr = VaStartInst->getArgList();

  auto VaListMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(VaListMemLocationMatr);

  bool IsValidMemLocationSeq = !VaListMemLocationSeq.empty();
  if (IsValidMemLocationSeq) {
    bool IsArrayDecay = DataFlowUtils::isArrayDecay(VaListMemLocationMatr);
    if (IsArrayDecay) {
      VaListMemLocationSeq.pop_back();
    }

    ExtendedValue EV(Fact);
    EV.setVaListMemLocationSeq(VaListMemLocationSeq);

    TargetFacts.insert(EV);

    LOG_DEBUG("Created new VarArg from template");
    LOG_DEBUG("Template");
    DataFlowUtils::dumpFact(Fact);
    LOG_DEBUG("VarArg");
    DataFlowUtils::dumpFact(EV);
  }

  return TargetFacts;
}

} // namespace psr
