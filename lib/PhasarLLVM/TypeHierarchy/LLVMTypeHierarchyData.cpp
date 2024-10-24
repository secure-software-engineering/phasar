/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchyData.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

namespace psr {

static LLVMTypeHierarchyData getDataFromJson(const nlohmann::json &Json) {
  LLVMTypeHierarchyData Data;
  Data.PhasarConfigJsonTypeHierarchyID =
      Json["PhasarConfigJsonTypeHierarchyID"];

  for (const auto &[Key, ValueArray] :
       Json[Data.PhasarConfigJsonTypeHierarchyID].items()) {
    Data.TypeGraph.try_emplace(Key, std::vector<std::string>{});

    for (const auto &CurrInnerType : ValueArray) {
      for (const auto &CurrString : CurrInnerType) {
        Data.TypeGraph[Key].push_back(CurrString.get<std::string>());
      }
    }
  }

  return Data;
}

void LLVMTypeHierarchyData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json Json;

  Json["PhasarConfigJsonTypeHierarchyID"] = PhasarConfigJsonTypeHierarchyID;
  auto &JTH = Json[PhasarConfigJsonTypeHierarchyID];

  for (const auto &Curr : TypeGraph) {
    auto &DataPos = JTH[Curr.getKey()].emplace_back();

    for (const auto &CurrTypeName : Curr.getValue()) {
      DataPos.push_back(CurrTypeName);
    }
  }

  OS << Json << '\n';
}

LLVMTypeHierarchyData
LLVMTypeHierarchyData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

LLVMTypeHierarchyData
LLVMTypeHierarchyData::loadJsonString(llvm::StringRef JsonAsString) {
  nlohmann::json Data =
      nlohmann::json::parse(JsonAsString.begin(), JsonAsString.end());
  return getDataFromJson(Data);
}

} // namespace psr
