#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"

namespace psr::LCUtils {
class JoinEdgeFunction
    : public psr::EdgeFunction<IDELinearConstantPropagation::v_t>,
      public std::enable_shared_from_this<JoinEdgeFunction> {
  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> frst;
  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> scnd;
  size_t maxSize;

public:
  JoinEdgeFunction(
      const std::shared_ptr<
          psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &frst,
      const std::shared_ptr<
          psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &scnd,
      size_t maxSize);
  IDELinearConstantPropagation::v_t
  computeTarget(IDELinearConstantPropagation::v_t source) override;

  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
  composeWith(
      std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
          secondFunction) override;

  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
  joinWith(std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
               otherFunction) override;

  bool
  equal_to(std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
               other) const override;
  void print(std::ostream &OS, bool isForDebug = false) const override;
  const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &
  getFirst() const;
  const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &
  getSecond() const;
};
} // namespace psr::LCUtils
