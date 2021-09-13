/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instruction.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinConstEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/JoinEdgeFunction.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"

namespace psr::XTaint {

JoinEdgeFunction::JoinEdgeFunction(BasicBlockOrdering &BBO,
                                   SubEdgeFuctionsTy &&subEF,
                                   const EdgeDomain &seed)
    : EdgeFunctionBase(Kind::Join, BBO), subEF(std::move(subEF)), seed(seed) {}
JoinEdgeFunction::JoinEdgeFunction(
    BasicBlockOrdering &BBO, std::initializer_list<EdgeFunctionPtrType> subEF,
    const EdgeDomain &seed)
    : EdgeFunctionBase(Kind::Join, BBO), subEF(subEF), seed(seed) {}

auto JoinEdgeFunction::create(BasicBlockOrdering &BBO,
                              EdgeFunctionPtrType First,
                              EdgeFunctionPtrType Second)
    -> EdgeFunctionPtrType {

  constexpr size_t SUB_EF_THRESHOLD = 5;

  // Don't handle GenEdgeFunction here explicitly, because it is already handled
  // in the joinWith(...) functions resulting in a JoinConstEdgeFunction;

  if (&*First == &*Second || First->equal_to(Second))
    return First;
  // Helper-function to handle the case where exactly one of {First, Second} is
  // a JoinEdgeFunction
  auto joinSingle =
      [&BBO](
          EdgeFunctionPtrType single, const JoinEdgeFunction *other,
          const EdgeFunctionPtrType &otherEF) mutable -> EdgeFunctionPtrType {
    if (other->subEF.count(single))
      return otherEF;

    if (other->subEF.size() == SUB_EF_THRESHOLD) {
      return getAllBot();
    }

    SubEdgeFuctionsTy subs;
    subs.reserve(1 + other->subEF.size());
    subs.insert(single);
    subs.insert(other->subEF.begin(), other->subEF.end());

    return makeEF<JoinEdgeFunction>(BBO, std::move(subs), other->seed);
  };

  EdgeDomain seed = psr::Top{};

  if (auto *firstJC = dynamic_cast<JoinConstEdgeFunction *>(&*First)) {
    if (auto *secondJC = dynamic_cast<JoinConstEdgeFunction *>(&*Second)) {
      seed = EdgeDomain(firstJC->getConstant()).join(secondJC->getConstant());
      First = firstJC->getFunction();
      Second = secondJC->getFunction();
    } else {
      seed = firstJC->getConstant();
      First = firstJC->getFunction();
    }
  } else if (auto *secondJC = dynamic_cast<JoinConstEdgeFunction *>(&*Second)) {
    seed = secondJC->getConstant();
    Second = secondJC->getFunction();
  }

  if (&*First == &*Second || First->equal_to(Second)) // Just to be sure...
    return First;

  if (auto *firstEF = dynamic_cast<JoinEdgeFunction *>(&*First)) {
    if (auto *secondEF = dynamic_cast<JoinEdgeFunction *>(&*Second)) {
      // Most difficult case: Both First and Second are JoinEdgeFunctions. Merge
      // the subEF-sets by using set-union (deduplicating)

      if (secondEF->subEF.size() < firstEF->subEF.size()) {
        std::swap(firstEF, secondEF);
      }
      SubEdgeFuctionsTy subs(secondEF->subEF);
      for (const auto &sub : firstEF->subEF) {
        subs.insert(sub);
      }

      if (subs.size() > SUB_EF_THRESHOLD)
        return getAllBot();

      return makeEF<JoinEdgeFunction>(BBO, std::move(subs),
                                      firstEF->seed.join(secondEF->seed, &BBO));
    } else {
      return joinSingle(Second, firstEF, First);
    }
  } else if (auto *secondEF = dynamic_cast<JoinEdgeFunction *>(&*Second)) {
    return joinSingle(First, secondEF, Second);

  } else {
    return makeEF<JoinEdgeFunction>(BBO, SubEdgeFuctionsTy{First, Second},
                                    psr::Top{});
  }
}

llvm::hash_code JoinEdgeFunction::getHashCode() const {
  assert(!subEF.empty());

  auto it = subEF.begin();
  auto frst = XTaint::getHashCode(*it);
  if (subEF.size() == 1)
    return frst;

  auto scnd = XTaint::getHashCode(*++it);
  return llvm::hash_combine(frst, scnd);
}

JoinEdgeFunction::l_t JoinEdgeFunction::computeTarget(l_t Source) {
  l_t ret = seed;
  for (const auto &sub : subEF) {
    ret = ret.join(sub->computeTarget(Source), &BBO);
    if (ret.isBottom() || ret.isNotSanitized())
      return ret;
  }

  return ret;
}

bool JoinEdgeFunction::equal_to(EdgeFunctionPtrType OtherFunction) const {
  assert(OtherFunction);

  if (this == &*OtherFunction)
    return true;

  if (auto OtherJoin = dynamic_cast<JoinEdgeFunction *>(&*OtherFunction)) {
    return seed == OtherJoin->seed && subEF == OtherJoin->subEF;
  }
  return false;
}

void JoinEdgeFunction::print(std::ostream &OS, bool IsForDebug) const {
  assert(!subEF.empty());
  auto it = subEF.begin();
  auto frst = *it;

  frst->print(OS << "JOIN[" << seed << ": ", IsForDebug);
  if (subEF.size() > 1) {
    auto scnd = *++it;
    scnd->print(OS << ", ", IsForDebug);
  }
  if (subEF.size() > 2) {
    OS << ", ... and " << (subEF.size() - 2) << " more";
  }

  OS << "]";
}

} // namespace psr::XTaint