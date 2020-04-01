/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CheckOperandsFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
CheckOperandsFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  for (const auto &use : currentInst->operands()) {
    const auto &operand = use.get();

    bool isOperandTainted =
        DataFlowUtils::isValueTainted(operand, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(operand, Fact);

    if (isOperandTainted) {
      traceStats.add(currentInst);

      return {Fact, ExtendedValue(currentInst)};
    }
  }

  return {Fact};
}

} // namespace psr
