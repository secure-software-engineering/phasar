#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"

using namespace psr;

static FunctionDataFlowFacts createLibCSummary() {
  FunctionDataFlowFacts Sum;
  // TODO
  return Sum;
}

const FunctionDataFlowFacts &psr::getLibCSummary() {
  static const auto Sum = createLibCSummary();
  return Sum;
}
