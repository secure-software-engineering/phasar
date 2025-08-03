/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTableData.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

namespace psr {

static LLVMVFTableData getDataFromJson(const nlohmann::json &Json) {
  LLVMVFTableData Data;

  for (const auto &Curr : Json["VFT"]) {
    Data.VFT.push_back(Curr.get<std::string>());
  }

  return Data;
}

void LLVMVFTableData::printAsJson(llvm::raw_ostream &OS) const {
  nlohmann::json JSON;

  for (const auto &Curr : VFT) {
    JSON["VFT"].emplace_back(Curr);
  }

  OS << JSON << '\n';
}

LLVMVFTableData LLVMVFTableData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

LLVMVFTableData LLVMVFTableData::loadJsonString(llvm::StringRef JsonAsString) {
  nlohmann::json Data =
      nlohmann::json::parse(JsonAsString.begin(), JsonAsString.end());
  return getDataFromJson(Data);
}

} // namespace psr
