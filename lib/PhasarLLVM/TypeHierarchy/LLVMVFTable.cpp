/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <iostream>
#include <utility>

#include "llvm/IR/Function.h"

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"

using namespace psr;

namespace psr {

const llvm::Function *LLVMVFTable::getFunction(unsigned Idx) const {
  if (Idx < size()) {
    return VFT[Idx];
  }
  return nullptr;
}

int LLVMVFTable::getIndex(const llvm::Function *F) const {
  auto It = std::find(VFT.begin(), VFT.end(), F);
  if (It != VFT.end()) {
    return std::distance(VFT.begin(), It);
  }
  return -1;
}

void LLVMVFTable::print(std::ostream &OS) const {
  for (const auto *F : VFT) {
    OS << F->getName().str();
    if (F != VFT.back()) {
      OS << '\n';
    }
  }
}

nlohmann::json LLVMVFTable::getAsJson() const {
  nlohmann::json J = "{}"_json;
  return J;
}

} // namespace psr
