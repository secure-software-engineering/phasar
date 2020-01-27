/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GEPInstFlowFunction.h>

namespace psr {

std::set<ExtendedValue>
GEPInstFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  const auto gepInst = llvm::cast<llvm::GetElementPtrInst>(currentInst);
  const auto gepInstPtr = gepInst->getPointerOperand();

  bool isVarArgFact = fact.isVarArg();
  if (isVarArgFact) {
    bool killFact = gepInstPtr->getName().contains_lower("reg_save_area");
    if (killFact)
      return {};

    bool incrementCurrentVarArgIndex =
        gepInst->getName().contains_lower("overflow_arg_area.next");
    if (incrementCurrentVarArgIndex) {
      const auto gepVaListMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(gepInstPtr);

      bool isVaListEqual = DataFlowUtils::isSubsetMemoryLocationSeq(
          DataFlowUtils::getVaListMemoryLocationSeqFromFact(fact),
          gepVaListMemLocationSeq);
      if (isVaListEqual) {
        ExtendedValue ev(fact);
        ev.incrementCurrentVarArgIndex();

        return {ev};
      }
    }
  } else {
    bool isPtrTainted = DataFlowUtils::isValueTainted(gepInstPtr, fact);
    if (isPtrTainted)
      return {fact, ExtendedValue(gepInst)};
  }

  return {fact};
}

} // namespace psr
