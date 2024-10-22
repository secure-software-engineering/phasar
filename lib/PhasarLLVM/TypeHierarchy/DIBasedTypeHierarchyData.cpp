/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchyData.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/StringRef.h"

#include <cstdint>
#include <string>
#include <utility>

namespace psr {

static DIBasedTypeHierarchyData getDataFromJson(const nlohmann::json &Json) {
  DIBasedTypeHierarchyData Data;

  Data.VertexTypes = Json["VertexTypes"].get<std::vector<std::string>>();

  Data.TransitiveDerivedIndex =
      Json["TransitiveDerivedIndex"]
          .get<std::vector<std::pair<uint32_t, uint32_t>>>();

  Data.Hierarchy = Json["Hierarchy"].get<std::vector<std::string>>();

  for (const auto &CurrVTable : Json["VTables"]) {
    auto &DataPos = Data.VTables.emplace_back();

    for (const auto &CurrVFunc : CurrVTable) {
      DataPos.push_back(CurrVFunc.get<std::string>());
    }
  }

  return Data;
}

void DIBasedTypeHierarchyData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json Json;

  Json["VertexTypes"] = VertexTypes;
  Json["TransitiveDerivedIndex"] = TransitiveDerivedIndex;
  Json["Hierarchy"] = Hierarchy;

  auto &JVTables = Json["VTables"];
  for (const auto &CurrVTable : VTables) {
    auto &DataPos = JVTables.emplace_back();

    for (const auto &CurrVFunc : CurrVTable) {
      DataPos.push_back(CurrVFunc);
    }
  }

  OS << Json << '\n';
}

DIBasedTypeHierarchyData
DIBasedTypeHierarchyData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

DIBasedTypeHierarchyData
DIBasedTypeHierarchyData::loadJsonString(llvm::StringRef JsonAsString) {
  nlohmann::json Data =
      nlohmann::json::parse(JsonAsString.begin(), JsonAsString.end());
  return getDataFromJson(Data);
}

} // namespace psr
