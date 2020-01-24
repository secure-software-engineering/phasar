#pragma once
#include <memory>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <z3++.h>
// Dummy for variability-aware edge function: Implementation will be added later

namespace psr {
template <typename L> class VariationalEdgeFunction : public EdgeFunction<L> {
public:
  VariationalEdgeFunction(const std::shared_ptr<EdgeFunction<L>> &userEF,
                          z3::expr cond) {}
  // TODO implement
  L computeTarget(L source) override { return source; }

  std::shared_ptr<EdgeFunction<L>>
  composeWith(std::shared_ptr<EdgeFunction<L>> secondFunction) override {
    return secondFunction;
  }

  std::shared_ptr<EdgeFunction<L>>
  joinWith(std::shared_ptr<EdgeFunction<L>> otherFunction) override {
    return otherFunction;
  }

  bool equal_to(std::shared_ptr<EdgeFunction<L>> other) const override {
    return false;
  }
};
} // namespace psr