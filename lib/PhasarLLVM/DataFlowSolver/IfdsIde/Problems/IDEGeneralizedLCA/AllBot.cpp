/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"

namespace psr {

std::shared_ptr<AllBot::type> AllBot::getInstance() {
  static std::shared_ptr<type> ret =
      std::make_shared<type>(IDEGeneralizedLCA::v_t{nullptr});
  return ret;
}

bool AllBot::isBot(const EdgeFunction<IDEGeneralizedLCA::v_t> *edgeFn,
                   bool nonRec) {
  if (edgeFn == nullptr)
    return false;
  if (edgeFn == getInstance().get())
    return true;
  if (dynamic_cast<const type *>(edgeFn))
    return true;
  if (!nonRec) {
    if (auto joinEFn = dynamic_cast<const JoinEdgeFunction *>(edgeFn))
      return isBot(joinEFn->getFirst(), true) &&
             isBot(joinEFn->getSecond(), true);
  }
  return false;
}

bool AllBot::isBot(
    const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &edgeFn,
    bool nonRec) {
  return isBot(edgeFn.get(), nonRec);
}

} // namespace psr
