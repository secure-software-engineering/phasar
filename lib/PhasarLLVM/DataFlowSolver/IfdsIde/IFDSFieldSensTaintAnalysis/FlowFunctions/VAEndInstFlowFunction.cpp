/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAEndInstFlowFunction.h>

#include <llvm/IR/IntrinsicInst.h>

namespace psr {

std::set<ExtendedValue>
VAEndInstFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  bool isVarArgFact = fact.isVarArg();
  if (!isVarArgFact)
    return {fact};

  const auto vaEndInst = llvm::cast<llvm::VAEndInst>(currentInst);
  const auto vaEndMemLocationMatr = vaEndInst->getArgList();

  auto vaEndMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(vaEndMemLocationMatr);

  bool isValidMemLocationSeq = !vaEndMemLocationSeq.empty();
  if (isValidMemLocationSeq) {
    bool isArrayDecay = DataFlowUtils::isArrayDecay(vaEndMemLocationMatr);
    if (isArrayDecay)
      vaEndMemLocationSeq.pop_back();

    bool isVaListEqual = DataFlowUtils::isMemoryLocationSeqsEqual(
        DataFlowUtils::getVaListMemoryLocationSeqFromFact(fact),
        vaEndMemLocationSeq);
    if (isVaListEqual) {
      LOG_DEBUG("Killed VarArg");
      DataFlowUtils::dumpFact(fact);

      return {};
    }
  }

  return {fact};
}

} // namespace psr
