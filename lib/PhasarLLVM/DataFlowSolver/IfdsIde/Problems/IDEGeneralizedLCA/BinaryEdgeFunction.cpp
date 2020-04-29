/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/BinaryEdgeFunction.h"
//#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

namespace psr {

IDEGeneralizedLCA::v_t
BinaryEdgeFunction::computeTarget(IDEGeneralizedLCA::v_t source) {
  /*auto ret = leftConst ? performBinOp(op, cnst, source, maxSize)
                       : performBinOp(op, source, cnst, maxSize);
  std::cout << "Binary(" << source << ") = " << ret << std::endl;
  return ret;*/
  if (leftConst) {
    return performBinOp(op, cnst, source, maxSize);
  } else {
    return performBinOp(op, source, cnst, maxSize);
  }
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
BinaryEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> secondFunction) {
  if (auto *EI = dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::v_t> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<AllBottom<IDEGeneralizedLCA::v_t> *>(secondFunction.get())) {
    // print(std::cout << "Compose ");
    // std::cout << " with ALLBOT" << std::endl;
    return shared_from_this();
  }
  return std::make_shared<LCAEdgeFunctionComposer>(this->shared_from_this(),
                                                   secondFunction, maxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
BinaryEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<IDEGeneralizedLCA::v_t> *>(otherFunction.get())) {
    return this->shared_from_this();
  }
  return std::make_shared<AllBottom<IDEGeneralizedLCA::v_t>>(
      IDEGeneralizedLCA::v_t({EdgeValue(nullptr)}));
}

bool BinaryEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> other) const {
  return this == other.get();
}

void BinaryEdgeFunction::print(std::ostream &OS, bool isForDebug) const {
  OS << "Binary_" << op;
}

} // namespace psr
