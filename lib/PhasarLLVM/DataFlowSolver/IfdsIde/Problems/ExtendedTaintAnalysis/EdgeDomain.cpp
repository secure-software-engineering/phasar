/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <iostream>

#include "llvm/Support/raw_os_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::XTaint {

EdgeDomain::EdgeDomain() : value(nullptr, Kind::Bot) {}

EdgeDomain::EdgeDomain(std::nullptr_t) : value(nullptr, Kind::WithSanitizer) {}

EdgeDomain::EdgeDomain(psr::Bottom) : value(nullptr, Kind::Bot) {}

EdgeDomain::EdgeDomain(psr::Top) : value(nullptr, Kind::Top) {}

EdgeDomain::EdgeDomain(psr::XTaint::Sanitized)
    : value(nullptr, Kind::Sanitized) {}

EdgeDomain::EdgeDomain(const llvm::Instruction *Sani)
    : value(Sani, Kind::WithSanitizer) {}

bool EdgeDomain::isBottom() const { return value.getInt() == Kind::Bot; }

bool EdgeDomain::isTop() const { return value.getInt() == Kind::Top; }

bool EdgeDomain::isSanitized() const {
  return value.getInt() == Kind::Sanitized;
}

bool EdgeDomain::isNotSanitized() const {
  return value.getInt() == Kind::WithSanitizer && value.getPointer() == nullptr;
}

bool EdgeDomain::hasSanitizer() const { return value.getPointer(); }

const llvm::Instruction *EdgeDomain::getSanitizer() const {
  return value.getPointer();
}

bool EdgeDomain::mayBeSanitized() const {
  return isSanitized() || hasSanitizer();
}

bool EdgeDomain::operator==(const EdgeDomain &Other) const {
  return Other.value == value;
}

bool EdgeDomain::operator<(const EdgeDomain &Other) const {
  return Other.value.getOpaqueValue() < value.getOpaqueValue();
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const EdgeDomain &ED) {
  switch (ED.getKind()) {
  case EdgeDomain::Bot:
    return OS << "Bottom";
  case EdgeDomain::Top:
    return OS << "Top";
  case EdgeDomain::Sanitized:
    return OS << "Sanitized";
  default:
    if (ED.getSanitizer())
      return OS << "WithSanitizer["
                << (ED.getSanitizer()->getType()->isVoidTy()
                        ? llvmIRToString(ED.getSanitizer())
                        : llvmIRToShortString(ED.getSanitizer()))
                << "]";
    else
      return OS << "NotSanitized";
  }
}

std::ostream &operator<<(std::ostream &OS, const EdgeDomain &ED) {
  llvm::raw_os_ostream osos(OS);
  osos << ED;
  return OS;
}

EdgeDomain EdgeDomain::join(const EdgeDomain &Other,
                            BasicBlockOrdering *BBO) const {
  auto ret = [&]() -> EdgeDomain {
    if (*this == Other)
      return *this;

    if (isTop())
      return Other;
    if (Other.isTop())
      return *this;

    if (isBottom() || Other.isBottom())
      return Bottom{};

    if (isSanitized())
      return Other; // Other is either NotSanitized or hasSanitizer
    if (Other.isSanitized())
      return *this; // this is either NotSanitized or hasSanitizer

    auto Sani1 = getSanitizer();
    auto Sani2 = Other.getSanitizer();

    if (!Sani1 || !Sani2)
      return nullptr;

    if (Sani1->getFunction() != Sani2->getFunction()) {
      // Don't know, whether Sani1 came before Sani2 or vice versa,
      // so kill them both here
      return nullptr;
    }

    if (BBO) {
      if (BBO->mustComeBefore(Sani1, Sani2))
        return Sani1;
      if (BBO->mustComeBefore(Sani2, Sani1))
        return Sani2;
    }

    llvm::SmallPtrSet<const llvm::Instruction *, 2> Succ;
    llvm::SmallVector<const llvm::Instruction *, 2> Intersection;
    for (auto succ : llvm::successors(Sani1->getParent())) {
      if (&succ->front() == Sani2)
        return Sani2; // result from previous join
      Succ.insert(&succ->front());
    }
    for (auto succ : llvm::successors(Sani2->getParent())) {
      if (&succ->front() == Sani1)
        return Sani2; // result from previous join

      if (Succ.count(&succ->front()))
        Intersection.push_back(&succ->front());
    }

    if (Intersection.size() == 1) {
      return Intersection
          .front(); // Move the sanitizer to the first common
                    // successor instruction. This is safe, since
                    // the (tainted) values from the branches
                    // cannot be used after the merge point,
                    // because they do not dominate the uses there.
    } else {
      return nullptr; // the only situation where this is possible is a while-
                      // or for loop
                      // => the loop body may not be executed at all => can
                      // safely kill the sanitizer here
    }
  }();

  return ret;
}

auto EdgeDomain::getKind() const -> Kind { return value.getInt(); }

} // namespace psr::XTaint