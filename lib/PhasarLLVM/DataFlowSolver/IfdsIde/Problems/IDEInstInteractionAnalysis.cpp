/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEInstInteractionAnalysis.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include <memory>
#include <tuple>

using namespace psr;

IDEIIAFlowFact::IDEIIAFlowFact(const llvm::Value *BaseVal) : Base(BaseVal) {}

IDEIIAFlowFact::IDEIIAFlowFact(
    const llvm::Value *BaseVal,
    llvm::SmallVector<const llvm::GetElementPtrInst *, Depth> FieldDesc)
    : Base(BaseVal), Field(std::move(FieldDesc)) {}

IDEIIAFlowFact IDEIIAFlowFact::create(const llvm::Value *BaseVal) {
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(BaseVal)) {
    return {BaseVal};
  }
  if (const auto *Gep = llvm::dyn_cast<llvm::GetElementPtrInst>(BaseVal)) {
    llvm::SmallVector<const llvm::GetElementPtrInst *, Depth> Field;
    Field.push_back(Gep);
    const llvm::Value *Base = Gep->getPointerOperand();
    while (const auto *NextGep = llvm::dyn_cast<llvm::GetElementPtrInst>(Base)
                                     ->getPointerOperand()) {
      Field.push_back(NextGep);
      Base = NextGep;
    }
    return {Base, Field};
  }
  llvm::report_fatal_error("Unexpected instruction!");
}

bool IDEIIAFlowFact::flowFactEqual(const IDEIIAFlowFact &Other) const {
  return false;
}

void IDEIIAFlowFact::print(llvm::raw_ostream &OS, bool IsForDebug) const {
  OS << "IDEIIAFlowFact { " << *Base;
  if (Field.empty()) {
    OS << " }";
  } else {
    OS << ", [";
    for (const auto *Gep : Field) {
      OS << *Gep;
      if (Gep != Field.back()) {
        OS << ", ";
      }
    }
    OS << "] }";
  }
}

bool IDEIIAFlowFact::operator==(const IDEIIAFlowFact &Other) const {
  if (Base != Other.Base) {
    return false;
  }
  for (unsigned Idx = 0; Idx < Field.size(); ++Idx) {
    if (Field[Idx] != Other.Field[Idx]) {
      return false;
    }
  }
  return true;
}

bool IDEIIAFlowFact::operator!=(const IDEIIAFlowFact &Other) const {
  return !(*this == Other);
}

bool IDEIIAFlowFact::operator<(const IDEIIAFlowFact &Other) const {
  return std::tie(Base, Field) < std::tie(Other.Base, Other.Field);
}

bool IDEIIAFlowFact::operator==(const llvm::Value *V) const {
  return Base == V;
}
