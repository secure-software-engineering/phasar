/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IdentityEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"

namespace psr {

/*IDEGeneralizedLCA::l_t
EdgeFunctionComposer::computeTarget(IDEGeneralizedLCA::l_t source) {
  auto ret = this->EdgeFunctionComposer<
      IDEGeneralizedLCA::l_t>::computeTarget(source);
  std::cout << "Compose(" << source << ") = " << ret << std::endl;
  return ret;
}*/

LCAEdgeFunctionComposer::LCAEdgeFunctionComposer(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> F,
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> G, size_t maxSize)
    : EdgeFunctionComposer<IDEGeneralizedLCA::l_t>(F, G), maxSize(maxSize) {}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
LCAEdgeFunctionComposer::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> secondFunction) {
  // see <phasar/PhasarLVM/IfdsIde/IDELinearConstantAnalysis.h>

  if (dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::l_t> *>(
          secondFunction.get()) ||
      dynamic_cast<AllBottom<IDEGeneralizedLCA::l_t> *>(secondFunction.get())) {
    return shared_from_this();
  }
  if (dynamic_cast<IdentityEdgeFunction *>(secondFunction.get())) {
    return shared_from_this();
  }
  if (dynamic_cast<GenConstant *>(secondFunction.get())) {
    return secondFunction;
  }
  auto gPrime = G->composeWith(secondFunction);
  if (gPrime->equal_to(G)) {
    return shared_from_this();
  }
  return F->composeWith(gPrime);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
LCAEdgeFunctionComposer::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> otherFunction) {
  // see <phasar/PhasarLVM/IfdsIde/IDELinearConstantAnalysis.h>
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<IDEGeneralizedLCA::l_t> *>(otherFunction.get())) {
    return this->shared_from_this();
  }
  if (AllBot::isBot(otherFunction)) {
    return AllBot::getInstance();
  }
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), otherFunction,
                                            maxSize);
}

const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &
LCAEdgeFunctionComposer::getFirst() const {
  return F;
}

const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &
LCAEdgeFunctionComposer::getSecond() const {
  return G;
}

} // namespace psr
