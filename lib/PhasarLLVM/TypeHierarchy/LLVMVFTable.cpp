/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTableData.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <utility>

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

void LLVMVFTable::print(llvm::raw_ostream &OS) const {
  for (const auto *F : VFT) {
    OS << F->getName();
    if (F != VFT.back()) {
      OS << '\n';
    }
  }
}

nlohmann::json LLVMVFTable::getAsJson() const {
  nlohmann::json J = "{}"_json;
  return J;
}

[[nodiscard]] LLVMVFTableData LLVMVFTable::getVFTableData() const {
  LLVMVFTableData Data;

  for (const auto &Curr : VFT) {
    if (Curr) {
      Data.VFT.push_back(Curr->getName().str());
      continue;
    }

    Data.VFT.emplace_back(NullFunName);
  }

  return Data;
}

void LLVMVFTable::printAsJson(llvm::raw_ostream &OS) const {
  LLVMVFTableData Data = getVFTableData();
  Data.printAsJson(OS);
}

std::vector<const llvm::Function *>
LLVMVFTable::getVFVectorFromIRVTable(const llvm::ConstantStruct &VT) {
  std::vector<const llvm::Function *> VFS;
  for (const auto &Op : VT.operands()) {
    if (const auto *CA = llvm::dyn_cast<llvm::ConstantArray>(Op)) {
      // Start iterating at offset 2, because offset 0 is vbase offset, offset 1
      // is RTTI
      for (const auto *It = std::next(CA->operands().begin(), 2);
           It != CA->operands().end(); ++It) {
        const auto *Entry = It->get()->stripPointerCastsAndAliases();

        const auto *F = llvm::dyn_cast<llvm::Function>(Entry);
        VFS.push_back(F);
      }
    }
  }
  return VFS;
}

} // namespace psr
