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

#include <llvm/IR/Function.h>

#include <phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h>

using namespace psr;

namespace psr {

LLVMVFTable::LLVMVFTable(std::vector<const llvm::Function *> Fs) : VFT(Fs) {}

const llvm::Function *LLVMVFTable::getFunction(unsigned Idx) const {
  if (Idx < size()) {
    return VFT[Idx];
  }
  return nullptr;
}

std::vector<const llvm::Function *> LLVMVFTable::getAllFunctions() const {
  return VFT;
}

int LLVMVFTable::getIndex(const llvm::Function *F) const {
  auto It = std::find(VFT.begin(), VFT.end(), F);
  if (It != VFT.end()) {
    return std::distance(VFT.begin(), It);
  }
  return -1;
}

bool LLVMVFTable::empty() const { return VFT.empty(); }

size_t LLVMVFTable::size() const { return VFT.size(); }

void LLVMVFTable::print(std::ostream &OS) const {
  for (auto F : VFT) {
    OS << F->getName().str();
    if (F != VFT.back()) {
      OS << '\n';
    }
  }
}

nlohmann::json LLVMVFTable::getAsJson() const {
  nlohmann::json j = "{}"_json;
  return j;
}

std::vector<const ::llvm::Function *>::iterator LLVMVFTable::begin() {
  return VFT.begin();
}

std::vector<const ::llvm::Function *>::const_iterator
LLVMVFTable::begin() const {
  return VFT.begin();
}

std::vector<const ::llvm::Function *>::iterator LLVMVFTable::end() {
  return VFT.end();
}

std::vector<const ::llvm::Function *>::const_iterator LLVMVFTable::end() const {
  return VFT.end();
}

} // namespace psr
