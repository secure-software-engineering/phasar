/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEInstInteractionAnalysis.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include <algorithm>
#include <memory>
#include <tuple>

using namespace psr;

IDEIIAFlowFact::IDEIIAFlowFact(const llvm::Value *BaseVal) : BaseVal(BaseVal) {}

IDEIIAFlowFact::IDEIIAFlowFact(
    const llvm::Value *BaseVal,
    llvm::SmallVector<const llvm::GetElementPtrInst *, KLimit> FieldDesc)
    : BaseVal(BaseVal), FieldDesc(std::move(FieldDesc)) {
  assert(FieldDesc.size() <= getKLimit() &&
         "Field descriptor exceeds k-limit!");
}

IDEIIAFlowFact IDEIIAFlowFact::create(const llvm::Value *BaseVal) {
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(BaseVal)) {
    return {BaseVal};
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(BaseVal)) {
    // Construct field descriptor
    llvm::SmallVector<const llvm::GetElementPtrInst *, KLimit> FieldDesc;
    FieldDesc.push_back(Gep);
    const auto *NextGep = Gep->getPointerOperand();
    while (llvm::isa_and_nonnull<llvm::GetElementPtrInst>(NextGep)) {
      Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(NextGep);
      if (FieldDesc.size() >= KLimit) {
        // do a left shift to make space
        if (FieldDesc.size() == 1) {
          FieldDesc.pop_back();
        } else {
          for (size_t Idx = 0; Idx < FieldDesc.size() - 1; ++Idx) {
            FieldDesc[Idx] = FieldDesc[Idx + 1];
          }
          FieldDesc.pop_back();
        }
      }
      FieldDesc.push_back(Gep);
      NextGep = Gep->getPointerOperand();
    }
    std::reverse(FieldDesc.begin(), FieldDesc.end());
    // Get base variable
    if (llvm::isa_and_nonnull<llvm::AllocaInst>(NextGep)) {
      BaseVal = NextGep;
    }
    assert(BaseVal && "BaseVal cannot be nullptr!");
    if (KLimit == 0) {
      return {BaseVal};
    }
    return {BaseVal, FieldDesc};
  }
  llvm::report_fatal_error("Unexpected instruction!");
}

bool IDEIIAFlowFact::flowFactEqual(const IDEIIAFlowFact &Other) const {
  return *this == Other;
}

void IDEIIAFlowFact::print(llvm::raw_ostream &OS,
                           [[maybe_unused]] bool IsForDebug) const {
  OS << "IDEIIAFlowFact { ";
  if (!BaseVal) {
    OS << "nullptr";
  } else {
    OS << *BaseVal;
  }
  if (FieldDesc.empty()) {
    OS << " }";
  } else {
    OS << ",\n\t[\n\t\t";
    for (const auto *Gep : FieldDesc) {
      OS << *Gep;
      if (Gep != FieldDesc.back()) {
        OS << ",\n\t\t";
      }
    }
    OS << "\n\t]\n}";
  }
}

bool IDEIIAFlowFact::operator==(const IDEIIAFlowFact &Other) const {
  if (BaseVal != Other.BaseVal) {
    return false;
  }
  if (FieldDesc.size() != Other.FieldDesc.size()) {
    return false;
  }
  auto EqualGEPDescriptor = [](const llvm::GetElementPtrInst *Lhs,
                               const llvm::GetElementPtrInst *Rhs) {
    const auto *LhsI = llvm::dyn_cast<llvm::Instruction>(Lhs);
    const auto *RhsI = llvm::dyn_cast<llvm::Instruction>(Rhs);
    return LhsI->isSameOperationAs(RhsI);
  };
  for (unsigned Idx = 0; Idx < FieldDesc.size(); ++Idx) {
    if (!EqualGEPDescriptor(FieldDesc[Idx], Other.FieldDesc[Idx])) {
      return false;
    }
  }
  return true;
}

bool IDEIIAFlowFact::operator!=(const IDEIIAFlowFact &Other) const {
  return !(*this == Other);
}

bool IDEIIAFlowFact::operator<(const IDEIIAFlowFact &Other) const {
  return std::tie(BaseVal, FieldDesc) <
         std::tie(Other.BaseVal, Other.FieldDesc);
}

bool IDEIIAFlowFact::operator==(const llvm::Value *V) const {
  return BaseVal == V && FieldDesc.empty();
}
