/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/JoinEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/AllBot.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/GenConstant.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"

namespace psr {

JoinEdgeFunction::JoinEdgeFunction(
    const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &First,
    const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &Second,
    size_t MaxSize)
    : First(First), Second(Second), MaxSize(MaxSize) {

  // check for endless recursion
  // This only used for debug purposes. So you can safely remove it, but you
  // also may use it, if there are termination problems

  std::unordered_set<EdgeFunction<IDEGeneralizedLCA::l_t> *> Seen;
  std::vector<EdgeFunction<IDEGeneralizedLCA::l_t> *> Q = {First.get(),
                                                           Second.get()};
  unsigned Ctr = 0;
  while (!Q.empty()) {
    auto *Top = Q.back();
    Ctr++;
    Q.pop_back();
    auto Ins = Seen.insert(Top);
    if (Top != nullptr && !Ins.second &&
        !dynamic_cast<AllBottom<IDEGeneralizedLCA::l_t> *>(Top)) {
      std::cerr << "WARNING: cyclic dependency! @" << Ctr << "#";
      Top->print(std::cerr);
      std::cerr << std::endl;
      this->First = this->Second = AllBot::getInstance();
      break;
    }
    if (auto *TopJoin = dynamic_cast<JoinEdgeFunction *>(Top)) {
      Q.push_back(TopJoin->First.get());
      Q.push_back(TopJoin->Second.get());
    } else if (auto *TopComp = dynamic_cast<LCAEdgeFunctionComposer *>(Top)) {
      Q.push_back(TopComp->getFirst().get());
      Q.push_back(TopComp->getSecond().get());
    }
  }
}
IDEGeneralizedLCA::l_t
JoinEdgeFunction::computeTarget(IDEGeneralizedLCA::l_t Source) {
  return join(First->computeTarget(Source), Second->computeTarget(Source),
              MaxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
JoinEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> SecondFunction) {
  // std::cout << "JoinFn composing" << std::endl;
  // TODO be more precise here
  if (dynamic_cast<GenConstant *>(SecondFunction.get())) {
    return SecondFunction;
  }
  if (dynamic_cast<EdgeIdentity<IDEGeneralizedLCA::l_t> *>(
          SecondFunction.get())) {
    return shared_from_this();
  }
  return std::make_shared<LCAEdgeFunctionComposer>(shared_from_this(),
                                                   SecondFunction, MaxSize);
}

std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>>
JoinEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> OtherFunction) {
  if (OtherFunction.get() == this) {
    return shared_from_this();
  }
  if (AllBot::isBot(OtherFunction)) {
    return AllBot::getInstance();
  }

  return std::make_shared<JoinEdgeFunction>(shared_from_this(), OtherFunction,
                                            MaxSize);
}

bool JoinEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> Other) const {

  if (this == Other.get()) {
    return true;
  }
  if (auto *OtherJoin = dynamic_cast<JoinEdgeFunction *>(Other.get())) {
    return (First->equal_to(OtherJoin->First) &&
            Second->equal_to(OtherJoin->Second)) // join is commutative...
           || (First->equal_to(OtherJoin->Second) &&
               Second->equal_to(OtherJoin->First));
  }
  return false;
}
void JoinEdgeFunction::print(std::ostream &OS, bool /*IsForDebug*/) const {
  OS << "JoinEdgeFn[";
  First->print(OS);
  OS << ", ";
  Second->print(OS);
  OS << "]";
}
const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &
JoinEdgeFunction::getFirst() const {
  return First;
}
const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::l_t>> &
JoinEdgeFunction::getSecond() const {
  return Second;
}

} // namespace psr
