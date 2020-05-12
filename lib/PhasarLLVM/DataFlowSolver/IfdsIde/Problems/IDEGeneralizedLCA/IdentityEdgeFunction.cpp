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

IdentityEdgeFunction::IdentityEdgeFunction(size_t MaxSize) : maxSize(MaxSize) {}
IDEGeneralizedLCA::l_t
IdentityEdgeFunction::computeTarget(IDEGeneralizedLCA::l_t Source) {
  return Source;
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IdentityEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction) {
  // std::cout << "Identity composing" << std::endl;
  return SecondFunction;
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
IdentityEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> OtherFunction) {
  // std::cout << "Identity join" << std::endl;
  // see <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
  if ((OtherFunction.get() == this) ||
      OtherFunction->equal_to(this->shared_from_this()))
    return this->shared_from_this();
  if (AllBot::isBot(OtherFunction))
    return AllBot::getInstance();
  if (auto At =
          dynamic_cast<AllTop<IDEGeneralizedLCA::l_t> *>(OtherFunction.get()))
    return this->shared_from_this();
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), OtherFunction,
                                            maxSize);
}

bool IdentityEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> Other) const {
  // see <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
  return Other.get() == this;
}
std::shared_ptr<IdentityEdgeFunction>
IdentityEdgeFunction::getInstance(size_t MaxSize) {
  // see <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>

  static std::unordered_map<size_t, std::shared_ptr<IdentityEdgeFunction>>
      Cache;
  auto It = Cache.find(MaxSize);
  if (It != Cache.end())
    return It->second;
  else
    return Cache[MaxSize] = std::make_shared<IdentityEdgeFunction>(MaxSize);

  // return std::make_shared<IdentityEdgeFunction>(maxSize);
}
void IdentityEdgeFunction::print(std::ostream &OS, bool IsForDebug) const {
  OS << "IdentityEdgeFn";
}

} // namespace psr
