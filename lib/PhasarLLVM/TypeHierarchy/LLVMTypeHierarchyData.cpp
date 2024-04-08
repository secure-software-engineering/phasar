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

  for (const auto &CurrOuterType : Json[Data.PhasarConfigJsonTypeHierarchyID]) {
    Data.TypeGraph.try_emplace(CurrOuterType.get<std::string>(),
                               std::vector<std::string>{});

    for (const auto &CurrInnerType : CurrOuterType) {
      Data.TypeGraph[CurrOuterType.get<std::string>()].push_back(
          CurrInnerType.get<std::string>());
    }
  }

  return Data;
}

void LLVMTypeHierarchyData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json Json;

  for (const auto &Curr : TypeGraph) {
    auto &DataPos =
        Json[PhasarConfigJsonTypeHierarchyID][Curr.getKey()].emplace_back();

    for (const auto &CurrTypeName : Curr.getValue()) {
      DataPos.push_back(CurrTypeName);
    }
  }

  OS << Json;
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
