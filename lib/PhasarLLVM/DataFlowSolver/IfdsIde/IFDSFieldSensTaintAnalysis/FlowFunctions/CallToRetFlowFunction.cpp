/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CallToRetFlowFunction.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
CallToRetFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  /*
   * Kill every global and expect the callee to return all valid ones.
   */
  bool IsGlobalMemLocationFact = DataFlowUtils::isGlobalMemoryLocationSeq(
      DataFlowUtils::getMemoryLocationSeqFromFact(Fact));
  if (IsGlobalMemLocationFact) {
    return {};
  }

  /*
   * For functions that kill facts and are handled in getSummaryFlowFunction()
   * we kill all facts here and just use what they have returned. This is
   * important e.g. if memset removes a store fact then it is not readded here
   * e.g. through identity function.
   *
   * Need to keep the list in sync with "killing" functions in
   * getSummaryFlowFunction()!
   */
  bool IsHandledInSummaryFlowFunction =
      llvm::isa<llvm::MemTransferInst>(CurrentInst) ||
      llvm::isa<llvm::MemSetInst>(CurrentInst) ||
      llvm::isa<llvm::VAEndInst>(CurrentInst);

  if (IsHandledInSummaryFlowFunction) {
    return {};
  }

  return {Fact};
}

} // namespace psr
