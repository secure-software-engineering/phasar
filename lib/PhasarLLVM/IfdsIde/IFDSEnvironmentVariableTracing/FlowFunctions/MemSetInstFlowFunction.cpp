/**
  * @author Sebastian Roland <seroland86@gmail.com>
  */

#include "MemSetInstFlowFunction.h"

#include <llvm/IR/IntrinsicInst.h>

namespace psr {

std::set<ExtendedValue>
MemSetInstFlowFunction::computeTargetsExt(ExtendedValue& fact)
{
  const auto memSetInst = llvm::cast<const llvm::MemSetInst>(currentInst);
  const auto dstMemLocationMatr = memSetInst->getRawDest();

  bool killFact = DataFlowUtils::isMemoryLocationTainted(dstMemLocationMatr, fact);
  if (killFact) {
    traceStats.add(memSetInst);

    return { };
  }

  return { fact };
}

} // namespace
