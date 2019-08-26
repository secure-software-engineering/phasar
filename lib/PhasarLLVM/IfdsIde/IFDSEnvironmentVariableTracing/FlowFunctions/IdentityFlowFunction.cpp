/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/IdentityFlowFunction.h>

namespace psr {

std::set<ExtendedValue>
IdentityFlowFunction::computeTargetsExt(ExtendedValue &fact) {
  return {fact};
}

} // namespace psr
