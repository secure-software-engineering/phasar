/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GenerateFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
GenerateFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  traceStats.add(currentInst);

  if (Fact == zeroValue) {
    return {ExtendedValue(currentInst)};
  }

  return {Fact};
}

} // namespace psr
