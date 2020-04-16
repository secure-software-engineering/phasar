#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"

namespace CCPP::LCUtils {
class IdentityEdgeFunction
    : public psr::EdgeFunction<IDELinearConstantPropagation::v_t>,
      public std::enable_shared_from_this<IdentityEdgeFunction> {
  size_t maxSize;

public:
  IdentityEdgeFunction(size_t maxSize);
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
  static std::shared_ptr<IdentityEdgeFunction> getInstance(size_t maxSize);
};
// typedef psr::EdgeIdentity<IDELinearConstantPropagation::v_t>
//   IdentityEdgeFunction;
} // namespace CCPP::LCUtils
