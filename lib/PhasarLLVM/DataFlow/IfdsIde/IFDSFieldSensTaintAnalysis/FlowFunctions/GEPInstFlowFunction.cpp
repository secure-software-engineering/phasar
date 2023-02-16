/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GEPInstFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"

#include "llvm/IR/Instructions.h"

namespace psr {

std::set<ExtendedValue>
GEPInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  const auto *const GepInst = llvm::cast<llvm::GetElementPtrInst>(CurrentInst);
  const auto *const GepInstPtr = GepInst->getPointerOperand();

  bool IsVarArgFact = Fact.isVarArg();
  if (IsVarArgFact) {
    bool KillFact = GepInstPtr->getName().contains("reg_save_area");
    if (KillFact) {
      return {};
    }

    bool IncrementCurrentVarArgIndex =
        GepInst->getName().contains("overflow_arg_area.next");
    if (IncrementCurrentVarArgIndex) {
      const auto GepVaListMemLocationSeq =
          DataFlowUtils::getMemoryLocationSeqFromMatr(GepInstPtr);

      bool IsVaListEqual = DataFlowUtils::isSubsetMemoryLocationSeq(
          DataFlowUtils::getVaListMemoryLocationSeqFromFact(Fact),
          GepVaListMemLocationSeq);
      if (IsVaListEqual) {
        ExtendedValue EV(Fact);
        EV.incrementCurrentVarArgIndex();

        return {EV};
      }
    }
  } else {
    bool IsPtrTainted = DataFlowUtils::isValueTainted(GepInstPtr, Fact);
    if (IsPtrTainted) {
      return {Fact, ExtendedValue(GepInst)};
    }
  }

  return {Fact};
}

} // namespace psr
