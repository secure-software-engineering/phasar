/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CheckOperandsFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
CheckOperandsFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  for (const auto &Use : CurrentInst->operands()) {
    const auto &Operand = Use.get();

    bool IsOperandTainted =
        DataFlowUtils::isValueTainted(Operand, Fact) ||
        DataFlowUtils::isMemoryLocationTainted(Operand, Fact);

    if (IsOperandTainted) {
      TStats.add(CurrentInst);

      return {Fact, ExtendedValue(CurrentInst)};
    }
  }

  return {Fact};
}

} // namespace psr
