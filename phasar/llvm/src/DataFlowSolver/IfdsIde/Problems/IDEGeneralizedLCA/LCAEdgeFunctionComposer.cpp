/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/GenConstant.h"
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
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> G, size_t MaxSize)
    : EdgeFunctionComposer<IDEGeneralizedLCA::l_t>(F, G), MaxSize(MaxSize) {}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
LCAEdgeFunctionComposer::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction) {
  // see <phasar/PhasarLVM/IfdsIde/IDELinearConstantAnalysis.h>

  if (dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::l_t> *>(
          SecondFunction.get()) ||
      dynamic_cast<AllBottom<IDEGeneralizedLCA::l_t> *>(SecondFunction.get())) {
    return shared_from_this();
  }
  if (dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::l_t> *>(
          SecondFunction.get())) {
    return shared_from_this();
  }
  if (dynamic_cast<GenConstant *>(SecondFunction.get())) {
    return SecondFunction;
  }
  auto GPrime = G->composeWith(SecondFunction);
  if (GPrime->equal_to(G)) {
    return shared_from_this();
  }
  return F->composeWith(GPrime);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
LCAEdgeFunctionComposer::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> OtherFunction) {
  // see <phasar/PhasarLVM/IfdsIde/IDELinearConstantAnalysis.h>
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<IDEGeneralizedLCA::l_t> *>(OtherFunction.get())) {
    return this->shared_from_this();
  }
  if (AllBot::isBot(OtherFunction)) {
    return AllBot::getInstance();
  }
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), OtherFunction,
                                            MaxSize);
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
