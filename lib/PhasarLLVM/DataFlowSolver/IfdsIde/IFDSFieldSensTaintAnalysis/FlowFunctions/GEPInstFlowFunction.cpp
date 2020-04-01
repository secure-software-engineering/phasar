/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GEPInstFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
GEPInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto gepInst = llvm::cast<llvm::GetElementPtrInst>(currentInst);
  const auto gepInstPtr = gepInst->getPointerOperand();

  bool isVarArgFact = Fact.isVarArg();
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
          DataFlowUtils::getVaListMemoryLocationSeqFromFact(Fact),
          gepVaListMemLocationSeq);
      if (isVaListEqual) {
        ExtendedValue ev(Fact);
        ev.incrementCurrentVarArgIndex();

        return {ev};
      }
    }
  } else {
    bool isPtrTainted = DataFlowUtils::isValueTainted(gepInstPtr, Fact);
    if (isPtrTainted)
      return {Fact, ExtendedValue(gepInst)};
  }

  return {Fact};
}

} // namespace psr
