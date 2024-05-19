/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/LLVMAliasSetData.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

namespace psr {

static LLVMAliasSetData getDataFromJson(const nlohmann::json &Json) {
  LLVMAliasSetData Data;

  for (const auto &Value : Json["AliasSets"]) {
    Data.AliasSets.push_back(Value.get<std::vector<std::string>>());
  }

  for (const auto &Value : Json["AnalyzedFunctions"]) {
    Data.AnalyzedFunctions.push_back(Value.get<std::string>());
  }

  return Data;
}

void LLVMAliasSetData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json JSON;

  for (const auto &Curr : AliasSets) {
    JSON["AliasSets"].push_back(Curr);
  }

  for (const auto &Curr : AnalyzedFunctions) {
    JSON["AnalyzedFunctions"].push_back(Curr);
  }

  OS << JSON << '\n';
}

LLVMAliasSetData LLVMAliasSetData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

LLVMAliasSetData
LLVMAliasSetData::loadJsonString(llvm::StringRef JsonAsString) {
  nlohmann::json Data =
      nlohmann::json::parse(JsonAsString.begin(), JsonAsString.end());
  return getDataFromJson(Data);
}

} // namespace psr
