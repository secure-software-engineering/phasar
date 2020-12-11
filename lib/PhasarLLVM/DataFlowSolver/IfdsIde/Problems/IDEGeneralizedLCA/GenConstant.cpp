/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <iostream>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"

namespace psr {

GenConstant::GenConstant(const IDEGeneralizedLCA::l_t &Val, size_t MaxSize)
    : val(Val), maxSize(MaxSize) {
  // TODO: remove this?
  /*std::cout << "GenConstant: {";
  bool frst = true;
  for (auto &elem : val) {
    if (frst)
      frst = false;
    else
      std::cout << ", ";
    std::cout << elem;
  }
  std::cout << "}" << std::endl;*/
}
IDEGeneralizedLCA::l_t
GenConstant::computeTarget(IDEGeneralizedLCA::l_t Source) {
  // std::cout << "GenConstant computation (" << source << ")"
  //         << " = " << val << std::endl;
  return val;
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> GenConstant::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction) {
  // std::cout << "GenConstant composing" << std::endl;
  if (dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::l_t> *>(
          SecondFunction.get()) ||
      dynamic_cast<AllBottom<IDEGeneralizedLCA::l_t> *>(SecondFunction.get())) {

    return shared_from_this();
  }
  if (dynamic_cast<GenConstant *>(SecondFunction.get()))
    return SecondFunction;
  // return std::make_shared<EdgeFunctionComposer>(shared_from_this(),
  //                                              secondFunction, maxSize);
  return std::make_shared<GenConstant>(SecondFunction->computeTarget(val),
                                       maxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> GenConstant::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> Other) {
  if (auto OtherConst = dynamic_cast<GenConstant *>(Other.get())) {
    switch (compare(val, OtherConst->val)) {
    case Ordering::Equal:
    case Ordering::Greater:
      return shared_from_this();
    case Ordering::Less:
      return Other;
    default:
      return std::make_shared<GenConstant>(join(val, OtherConst->val, maxSize),
                                           maxSize);
    }
  }
  if (AllBot::isBot(Other))
    return AllBot::getInstance();
  return std::make_shared<JoinEdgeFunction>(shared_from_this(), Other, maxSize);
}

bool GenConstant::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> Other) const {
  if (auto OtherConst = dynamic_cast<GenConstant *>(Other.get())) {
    return val == OtherConst->val && maxSize == OtherConst->maxSize;
  }
  return false;
}
void GenConstant::print(std::ostream &OS, bool IsForDebug) const {
  OS << "GenConstantEdgeFn(" << val << ")";
}

} // namespace psr
