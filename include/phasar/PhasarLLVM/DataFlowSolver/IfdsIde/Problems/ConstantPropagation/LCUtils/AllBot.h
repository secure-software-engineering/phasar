#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/IDELinearConstantPropagation.h"

namespace psr::LCUtils {
struct AllBot {
  using type = psr::AllBottom<IDELinearConstantPropagation::v_t>;
  static std::shared_ptr<type> getInstance();
  static bool
  isBot(const psr::EdgeFunction<IDELinearConstantPropagation::v_t> *edgeFn,
        bool nonRec = false);
  static bool
  isBot(const std::shared_ptr<
            psr::EdgeFunction<IDELinearConstantPropagation::v_t>> &edgeFn,
        bool nonRec = false);
};

} // namespace psr::LCUtils