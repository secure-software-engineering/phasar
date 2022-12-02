/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/BinaryEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

namespace psr {

IDEGeneralizedLCA::l_t
BinaryEdgeFunction::computeTarget(IDEGeneralizedLCA::l_t Source) {
  /*auto ret = leftConst ? performBinOp(op, cnst, source, MaxSize)
                       : performBinOp(op, source, cnst, MaxSize);
  std::cout << "Binary(" << source << ") = " << ret << std::endl;
  return ret;*/
  if (LeftConst) {
    return performBinOp(Op, Const, Source, MaxSize);
  }
  return performBinOp(Op, Source, Const, MaxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
BinaryEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction) {
  if (auto *EI = dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::l_t> *>(
          SecondFunction.get())) {
    return this->shared_from_this();
  }
  if (dynamic_cast<AllBottom<IDEGeneralizedLCA::l_t> *>(SecondFunction.get())) {
    // print(std::cout << "Compose ");
    // std::cout << " with ALLBOT" << std::endl;
    return SecondFunction;
  }
  return std::make_shared<LCAEdgeFunctionComposer>(this->shared_from_this(),
                                                   SecondFunction, MaxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
BinaryEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> OtherFunction) {
  if (OtherFunction.get() == this ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<IDEGeneralizedLCA::l_t> *>(OtherFunction.get())) {
    return this->shared_from_this();
  }
  return std::make_shared<AllBottom<IDEGeneralizedLCA::l_t>>(
      IDEGeneralizedLCA::l_t({EdgeValue(nullptr)}));
}

bool BinaryEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> Other) const {
  return this == Other.get();
}

void BinaryEdgeFunction::print(llvm::raw_ostream &OS,
                               bool /*IsForDebug*/) const {
  OS << "Binary_" << Op;
}

} // namespace psr
