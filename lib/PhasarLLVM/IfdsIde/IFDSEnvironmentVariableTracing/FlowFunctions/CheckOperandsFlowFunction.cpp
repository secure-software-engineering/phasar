/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#include "CheckOperandsFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
CheckOperandsFlowFunction::computeTargetsExt(ExtendedValue& fact)
{
  for (const auto& use : currentInst->operands()) {
    const auto& operand = use.get();

    bool isOperandTainted = DataFlowUtils::isValueTainted(operand, fact) ||
                            DataFlowUtils::isMemoryLocationTainted(operand, fact);

    if (isOperandTainted) {
      traceStats.add(currentInst);

      return { fact, ExtendedValue(currentInst) };
    }
  }

  return { fact };
}

} // namespace
