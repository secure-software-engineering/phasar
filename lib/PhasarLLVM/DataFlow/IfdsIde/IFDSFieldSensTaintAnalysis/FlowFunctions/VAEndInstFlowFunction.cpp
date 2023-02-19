/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAEndInstFlowFunction.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
VAEndInstFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  bool IsVarArgFact = Fact.isVarArg();
  if (!IsVarArgFact) {
    return {Fact};
  }

  const auto *const VaEndInst = llvm::cast<llvm::VAEndInst>(CurrentInst);
  auto *const VaEndMemLocationMatr = VaEndInst->getArgList();

  auto VaEndMemLocationSeq =
      DataFlowUtils::getMemoryLocationSeqFromMatr(VaEndMemLocationMatr);

  bool IsValidMemLocationSeq = !VaEndMemLocationSeq.empty();
  if (IsValidMemLocationSeq) {
    bool IsArrayDecay = DataFlowUtils::isArrayDecay(VaEndMemLocationMatr);
    if (IsArrayDecay) {
      VaEndMemLocationSeq.pop_back();
    }

    bool IsVaListEqual = DataFlowUtils::isMemoryLocationSeqsEqual(
        DataFlowUtils::getVaListMemoryLocationSeqFromFact(Fact),
        VaEndMemLocationSeq);
    if (IsVaListEqual) {
      LOG_DEBUG("Killed VarArg");
      DataFlowUtils::dumpFact(Fact);

      return {};
    }
  }

  return {Fact};
}

} // namespace psr
