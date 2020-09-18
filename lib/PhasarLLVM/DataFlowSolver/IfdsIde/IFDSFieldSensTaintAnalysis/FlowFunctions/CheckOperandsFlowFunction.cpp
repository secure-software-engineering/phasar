/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CheckOperandsFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
CheckOperandsFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  for (const auto &Use : currentInst->operands()) {
    const auto &Operand = Use.get();

    bool IsOperandTainted =
        DataFlowUtils::isValueTainted(Operand, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(Operand, Fact);

    if (IsOperandTainted) {
      traceStats.add(currentInst);

      return {Fact, ExtendedValue(currentInst)};
    }
  }

  return {Fact};
}

} // namespace psr
