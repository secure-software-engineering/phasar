/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAStartInstFlowFunction.h>

#include <llvm/IR/IntrinsicInst.h>

namespace psr {

std::set<ExtendedValue>
VAStartInstFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  std::set<ExtendedValue> targetFacts;
  targetFacts.insert(fact);

  bool isVarArgTemplateFact = fact.isVarArgTemplate();
  if (!isVarArgTemplateFact)
    return targetFacts;

  const auto vaStartInst = llvm::cast<llvm::VAStartInst>(currentInst);
  const auto vaListMemLocationMatr = vaStartInst->getArgList();

  auto vaListMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(vaListMemLocationMatr);

  bool isValidMemLocationSeq = !vaListMemLocationSeq.empty();
  if (isValidMemLocationSeq) {
    bool isArrayDecay = DataFlowUtils::isArrayDecay(vaListMemLocationMatr);
    if (isArrayDecay)
      vaListMemLocationSeq.pop_back();

    ExtendedValue ev(fact);
    ev.setVaListMemLocationSeq(vaListMemLocationSeq);

    targetFacts.insert(ev);

    LOG_DEBUG("Created new VarArg from template");
    LOG_DEBUG("Template");
    DataFlowUtils::dumpFact(fact);
    LOG_DEBUG("VarArg");
    DataFlowUtils::dumpFact(ev);
  }

  return targetFacts;
}

} // namespace psr
