/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/IdentityFlowFunction.h>

namespace psr {

std::set<ExtendedValue>
IdentityFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  return {fact};
}

} // namespace psr
