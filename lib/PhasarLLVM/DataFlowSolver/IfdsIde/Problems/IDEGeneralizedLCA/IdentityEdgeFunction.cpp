/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <unordered_map>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IdentityEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"

namespace psr {

IdentityEdgeFunction::IdentityEdgeFunction(size_t maxSize) : maxSize(maxSize) {}
IDEGeneralizedLCA::v_t
IdentityEdgeFunction::computeTarget(IDEGeneralizedLCA::v_t source) {
  return source;
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
IdentityEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
        secondFunction) {
  // std::cout << "Identity composing" << std::endl;
  return secondFunction;
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
IdentityEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
        otherFunction) {
  // std::cout << "Identity join" << std::endl;
  // see <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
  if ((otherFunction.get() == this) ||
      otherFunction->equal_to(this->shared_from_this()))
    return this->shared_from_this();
  if (AllBot::isBot(otherFunction))
    return AllBot::getInstance();
  if (auto at = dynamic_cast<AllTop<IDEGeneralizedLCA::v_t> *>(
          otherFunction.get()))
    return this->shared_from_this();
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), otherFunction,
                                            maxSize);
}

bool IdentityEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> other)
    const {
  // see <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
  return other.get() == this;
}
std::shared_ptr<IdentityEdgeFunction>
IdentityEdgeFunction::getInstance(size_t maxSize) {
  // see <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>

  static std::unordered_map<size_t, std::shared_ptr<IdentityEdgeFunction>>
      cache;
  auto it = cache.find(maxSize);
  if (it != cache.end())
    return it->second;
  else
    return cache[maxSize] = std::make_shared<IdentityEdgeFunction>(maxSize);

  // return std::make_shared<IdentityEdgeFunction>(maxSize);
}
void IdentityEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "IdentityEdgeFn";
}

} // namespace psr
