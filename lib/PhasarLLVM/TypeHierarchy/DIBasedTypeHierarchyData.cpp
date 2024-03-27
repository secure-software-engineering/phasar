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

namespace psr {

static DIBasedTypeHierarchyData getDataFromJson(const nlohmann::json &Json) {
  DIBasedTypeHierarchyData ToReturn;

  /// TODO:

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
    JSON["VTables"].push_back(CurrVTable);
  }

  OS << JSON;
}

DIBasedTypeHierarchyData
DIBasedTypeHierarchyData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

DIBasedTypeHierarchyData
DIBasedTypeHierarchyData::loadJsonString(llvm::StringRef JsonAsString) {
  // nlohmann::json::parse needs a std::string, llvm::Twine won't work
  nlohmann::json ToStringify =
      nlohmann::json::parse(JsonAsString.begin(), JsonAsString.end());
  return getDataFromJson(ToStringify);
}

} // namespace psr
