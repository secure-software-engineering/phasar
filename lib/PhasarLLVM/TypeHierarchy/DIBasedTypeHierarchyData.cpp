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

#include <nlohmann/json_fwd.hpp>

namespace psr {

static DIBasedTypeHierarchyData getDataFromJson(const nlohmann::json &Json) {
  DIBasedTypeHierarchyData ToReturn;

  return ToReturn;
}

void DIBasedTypeHierarchyData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json JSON;

  for (const auto &Curr : NameToType) {
    JSON["NameToType"][Curr.first()] = Curr.second;
  }

  for (const auto &Curr : TypeToVertex) {
    JSON["TypeToVertex"][Curr.first] = Curr.second;
  }

  for (const auto &Curr : VertexTypes) {
    JSON["VertexTypes"].push_back(Curr);
  }

  for (const auto &Curr : TransitiveDerivedIndex) {
    JSON["TransitiveDerivedIndex"][Curr.first] = Curr.second;
  }

  for (const auto &Curr : Hierarchy) {
    JSON["Hierarchy"].push_back(Curr);
  }

  for (const auto &CurrVTable : VTables) {
    std::vector<std::string> ToAdd;
    ToAdd.reserve(CurrVTable.size());

    for (const auto &Curr : CurrVTable) {
      ToAdd.push_back(Curr);
    }

    JSON["VTables"].push_back(ToAdd);
  }

  OS << JSON;
}

DIBasedTypeHierarchyData
DIBasedTypeHierarchyData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

DIBasedTypeHierarchyData
DIBasedTypeHierarchyData::loadJsonString(const std::string &JsonAsString) {
  // nlohmann::json::parse needs a std::string, llvm::Twine won't work
  nlohmann::json ToStringify = nlohmann::json::parse(JsonAsString);
  return getDataFromJson(ToStringify);
}

} // namespace psr
