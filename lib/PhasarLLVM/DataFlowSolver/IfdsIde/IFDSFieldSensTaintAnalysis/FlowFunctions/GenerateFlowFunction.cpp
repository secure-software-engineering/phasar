/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GenerateFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
GenerateFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  traceStats.add(currentInst);

  if (fact == zeroValue)
    return {ExtendedValue(currentInst)};

  return {fact};
}

} // namespace psr
