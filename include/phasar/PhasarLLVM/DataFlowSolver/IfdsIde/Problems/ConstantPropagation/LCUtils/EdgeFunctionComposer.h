#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValueSet.h"

namespace psr::LCUtils {
class EdgeFunctionComposer
    : public psr::EdgeFunctionComposer<IDELinearConstantPropagation::v_t> {
  size_t maxSize;

public:
  EdgeFunctionComposer(
      std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> F,
      std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> G,
      size_t maxSize);
  // IDELinearConstantPropagation::v_t
  // computeTarget(IDELinearConstantPropagation::v_t source) override;
  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
  composeWith(
      std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
          secondFunction) override;

  std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
  joinWith(std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>>
               otherFunction) override;
  const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &
  getFirst() const;
  const std::shared_ptr<psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &
  getSecond() const;
};
} // namespace psr::LCUtils