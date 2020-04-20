#pragma once

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr {

struct AllBot {
  using type = AllBottom<IDEGeneralizedLCA::v_t>;
  static std::shared_ptr<type> getInstance();
  static bool isBot(const EdgeFunction<IDEGeneralizedLCA::v_t> *edgeFn,
                    bool nonRec = false);
  static bool
  isBot(const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &edgeFn,
        bool nonRec = false);
};

} // namespace psr
