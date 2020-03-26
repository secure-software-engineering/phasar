/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CallToRetFlowFunction.h"

#include "llvm/IR/IntrinsicInst.h"

namespace psr {

std::set<ExtendedValue>
CallToRetFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  /*
   * Kill every global and expect the callee to return all valid ones.
   */
  bool isGlobalMemLocationFact = DataFlowUtils::isGlobalMemoryLocationSeq(
      DataFlowUtils::getMemoryLocationSeqFromFact(fact));
  if (isGlobalMemLocationFact)
    return {};

  /*
   * For functions that kill facts and are handled in getSummaryFlowFunction()
   * we kill all facts here and just use what they have returned. This is
   * important e.g. if memset removes a store fact then it is not readded here
   * e.g. through identity function.
   *
   * Need to keep the list in sync with "killing" functions in
   * getSummaryFlowFunction()!
   */
  bool isHandledInSummaryFlowFunction =
      llvm::isa<llvm::MemTransferInst>(currentInst) ||
      llvm::isa<llvm::MemSetInst>(currentInst) ||
      llvm::isa<llvm::VAEndInst>(currentInst);

  if (isHandledInSummaryFlowFunction)
    return {};

  return {fact};
}

} // namespace psr
