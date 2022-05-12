/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/Support/raw_os_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::XTaint {

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const EdgeDomain &ED) {
  switch (ED.getKind()) {
  case EdgeDomain::Bot:
    return OS << "Bottom";
  case EdgeDomain::Top:
    return OS << "Top";
  case EdgeDomain::Sanitized:
    return OS << "Sanitized";
  default:
    if (ED.getSanitizer()) {
      return OS << "WithSanitizer["
                << (ED.getSanitizer()->getType()->isVoidTy()
                        ? llvmIRToString(ED.getSanitizer())
                        : llvmIRToShortString(ED.getSanitizer()))
                << "]";
    } else {
      return OS << "NotSanitized";
    }
  }
}

std::ostream &operator<<(std::ostream &OS, const EdgeDomain &ED) {
  llvm::raw_os_ostream ROS(OS);
  ROS << ED;
  return OS;
}

EdgeDomain EdgeDomain::join(const EdgeDomain &Other,
                            BasicBlockOrdering *BBO) const {

  if (*this == Other) {
    return *this;
  }

  if (isTop()) {
    return Other;
  }
  if (Other.isTop()) {
    return *this;
  }

  if (isBottom() || Other.isBottom()) {
    return Bottom{};
  }

  if (isSanitized()) {
    return Other; // Other is either NotSanitized or hasSanitizer
  }
  if (Other.isSanitized()) {
    return *this; // this is either NotSanitized or hasSanitizer
  }

  const auto *Sani1 = getSanitizer();
  const auto *Sani2 = Other.getSanitizer();

  if (!Sani1 || !Sani2) {
    return nullptr;
  }

  if (Sani1->getFunction() != Sani2->getFunction()) {
    // Don't know, whether Sani1 came before Sani2 or vice versa,
    // so kill them both here
    return nullptr;
  }

  if (BBO) {
    if (BBO->mustComeBefore(Sani1, Sani2)) {
      return Sani1;
    }
    if (BBO->mustComeBefore(Sani2, Sani1)) {
      return Sani2;
    }
  }

  llvm::SmallPtrSet<const llvm::Instruction *, 2> Succs;
  llvm::SmallVector<const llvm::Instruction *, 2> Intersection;
  for (const auto *Succ : llvm::successors(Sani1->getParent())) {
    if (&Succ->front() == Sani2) {
      return Sani2; // result from previous join
    }
    Succs.insert(&Succ->front());
  }
  for (const auto *Succ : llvm::successors(Sani2->getParent())) {
    if (&Succ->front() == Sani1) {
      return Sani2; // result from previous join
    }

    if (Succs.count(&Succ->front())) {
      Intersection.push_back(&Succ->front());
    }
  }

  if (Intersection.size() == 1) {
    // Move the sanitizer to the first common
    // successor instruction. This is safe, since
    // the (tainted) Values from the branches
    // cannot be used after the merge point,
    // because they do not dominate the uses there.
    return Intersection.front();
  }

  // the only situation where this is possible is a while-
  // or for loop
  // => the loop body may not be executed at all => can
  // safely kill the sanitizer here
  return nullptr;
}

} // namespace psr::XTaint
