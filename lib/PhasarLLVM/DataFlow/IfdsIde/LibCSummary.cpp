#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LibCSummary.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"

using namespace psr;

static FunctionDataFlowFacts createLibCSummary() {
  FunctionDataFlowFacts Sum;

  // TODO: Use public API instead!
  Sum.fdff["localtime_r"][1].emplace_back(ReturnValue{});

  // Sum.fdff["foo"][2].emplace_back(Parameter{1});

  // TODO
  return Sum;
}

const FunctionDataFlowFacts &psr::getLibCSummary() {
  static const auto Sum = createLibCSummary();
  return Sum;
}
