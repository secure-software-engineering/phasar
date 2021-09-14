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
                                   SubEdgeFuctionsTy &&SubEF,
                                   const EdgeDomain &Seed)
    : EdgeFunctionBase(Kind::Join, BBO), SubEF(std::move(SubEF)), Seed(Seed) {}
JoinEdgeFunction::JoinEdgeFunction(
    BasicBlockOrdering &BBO, std::initializer_list<EdgeFunctionPtrType> SubEF,
    const EdgeDomain &Seed)
    : EdgeFunctionBase(Kind::Join, BBO), SubEF(SubEF), Seed(Seed) {}

auto JoinEdgeFunction::create(BasicBlockOrdering &BBO,
                              EdgeFunctionPtrType First,
                              EdgeFunctionPtrType Second)
    -> EdgeFunctionPtrType {

  constexpr size_t SubEFThreshold = 5;

  // Don't handle GenEdgeFunction here explicitly, because it is already handled
  // in the joinWith(...) functions resulting in a JoinConstEdgeFunction;

  if (&*First == &*Second || First->equal_to(Second)) {
    return First;
  }
  // Helper-function to handle the case where exactly one of {First, Second} is
  // a JoinEdgeFunction
  auto joinSingle =
      [&BBO](
          const EdgeFunctionPtrType &Single, const JoinEdgeFunction *Other,
          const EdgeFunctionPtrType &OtherEF) mutable -> EdgeFunctionPtrType {
    if (Other->SubEF.count(Single)) {
      return OtherEF;
    }

    if (Other->SubEF.size() == SubEFThreshold) {
      return getAllBot();
    }

    SubEdgeFuctionsTy Subs;
    Subs.reserve(1 + Other->SubEF.size());
    Subs.insert(Single);
    Subs.insert(Other->SubEF.begin(), Other->SubEF.end());

    return makeEF<JoinEdgeFunction>(BBO, std::move(Subs), Other->Seed);
  };

  EdgeDomain Seed = psr::Top{};

  if (auto *FirstJC = dynamic_cast<JoinConstEdgeFunction *>(&*First)) {
    if (auto *SecondJC = dynamic_cast<JoinConstEdgeFunction *>(&*Second)) {
      Seed = EdgeDomain(FirstJC->getConstant()).join(SecondJC->getConstant());
      First = FirstJC->getFunction();
      Second = SecondJC->getFunction();
    } else {
      Seed = FirstJC->getConstant();
      First = FirstJC->getFunction();
    }
  } else if (auto *SecondJC = dynamic_cast<JoinConstEdgeFunction *>(&*Second)) {
    Seed = SecondJC->getConstant();
    Second = SecondJC->getFunction();
  }

  if (&*First == &*Second || First->equal_to(Second)) // Just to be sure...
    return First;

  if (auto *FirstEF = dynamic_cast<JoinEdgeFunction *>(&*First)) {
    if (auto *SecondEF = dynamic_cast<JoinEdgeFunction *>(&*Second)) {
      // Most difficult case: Both First and Second are JoinEdgeFunctions. Merge
      // the subEF-sets by using set-union (deduplicating)

      if (SecondEF->SubEF.size() < FirstEF->SubEF.size()) {
        std::swap(FirstEF, SecondEF);
      }
      SubEdgeFuctionsTy Subs(SecondEF->SubEF);
      for (const auto &Sub : FirstEF->SubEF) {
        Subs.insert(Sub);
      }

      if (Subs.size() > SubEFThreshold) {
        return getAllBot();
      }

      return makeEF<JoinEdgeFunction>(BBO, std::move(Subs),
                                      FirstEF->Seed.join(SecondEF->Seed, &BBO));
    }

    return joinSingle(Second, FirstEF, First);
  }
  if (auto *SecondEF = dynamic_cast<JoinEdgeFunction *>(&*Second)) {
    return joinSingle(First, SecondEF, Second);
  }

  return makeEF<JoinEdgeFunction>(BBO, SubEdgeFuctionsTy{First, Second},
                                  psr::Top{});
}

llvm::hash_code JoinEdgeFunction::getHashCode() const {
  assert(!SubEF.empty());

  auto It = SubEF.begin();
  auto Frst = XTaint::getHashCode(*It);
  if (SubEF.size() == 1) {
    return Frst;
  }

  auto Scnd = XTaint::getHashCode(*++It);
  return llvm::hash_combine(Frst, Scnd);
}

JoinEdgeFunction::l_t JoinEdgeFunction::computeTarget(l_t Source) {
  l_t Ret = Seed;
  for (const auto &Sub : SubEF) {
    Ret = Ret.join(Sub->computeTarget(Source), &BBO);
    if (Ret.isBottom() || Ret.isNotSanitized()) {
      return Ret;
    }
  }

  return Ret;
}

bool JoinEdgeFunction::equal_to(EdgeFunctionPtrType OtherFunction) const {
  assert(OtherFunction);

  if (this == &*OtherFunction) {
    return true;
  }

  if (auto *OtherJoin = dynamic_cast<JoinEdgeFunction *>(&*OtherFunction)) {
    return Seed == OtherJoin->Seed && SubEF == OtherJoin->SubEF;
  }
  return false;
}

void JoinEdgeFunction::print(std::ostream &OS, bool IsForDebug) const {
  assert(!SubEF.empty());
  auto It = SubEF.begin();
  auto Frst = *It;

  Frst->print(OS << "JOIN[" << Seed << ": ", IsForDebug);
  if (SubEF.size() > 1) {
    auto Scnd = *++It;
    Scnd->print(OS << ", ", IsForDebug);
  }
  if (SubEF.size() > 2) {
    OS << ", ... and " << (SubEF.size() - 2) << " more";
  }

  OS << "]";
}

} // namespace psr::XTaint