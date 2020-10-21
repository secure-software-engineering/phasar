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

void LLVMVFTable::setEntry(const unsigned Idx, const llvm::Function *F) {
  VFT[Idx] = F;
}

const llvm::Function *LLVMVFTable::getFunction(unsigned Idx) const {
  auto Search = VFT.find(Idx);
  if (Search != VFT.end()) {
    return Search->second;
  }
  return nullptr;
}

int LLVMVFTable::getIndex(const llvm::Function *F) const {
  for (auto &[Idx, Fun] : VFT) {
    if (Fun == F) {
      return Idx;
    }
  }
  return -1;
}

void LLVMVFTable::print(std::ostream &OS) const {
  for (const auto &[Idx, F] : VFT) {
    OS << Idx << ":" << F->getName().str() << '\n';
  }
}

nlohmann::json LLVMVFTable::getAsJson() const {
  nlohmann::json J = "{}"_json;
  return J;
}

} // namespace psr
