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

#include <string>

namespace psr {

static DIBasedTypeHierarchyData getDataFromJson(const nlohmann::json &Json) {
  DIBasedTypeHierarchyData Data;

  // NameToType
  for (const auto &[Key, Value] :
       Json["NameToType"].get<nlohmann::json::object_t>()) {
    Data.NameToType.try_emplace(Key, Value);
  }

  // TypeToVertex
  // Data.TypeToVertex.reserve(
  //    Json["TypeToVertex"].get<nlohmann::json::object_t>().size());
  for (const auto &[Key, Value] :
       Json["TypeToVertex"].get<nlohmann::json::object_t>()) {
    Data.TypeToVertex.try_emplace(Key, Value);
  }

  // VertexTypes
  for (const auto &Value : Json["VertexTypes"]) {
    Data.VertexTypes.push_back(Value);
  }

  // TransitiveDerivedIndex
  for (const auto &[First, Second] :
       Json["TransitiveDerivedIndex"].get<nlohmann::json::object_t>()) {
    // Data.TransitiveDerivedIndex.emplace_back(First, Second);
  }
  // Hierarchy
  for (const auto &Value : Json["Hierarchy"]) {
    Data.Hierarchy.push_back(Value);
  }
  // VTables
  /// TODO: Fabian fragen, wie ich hier am besten mit der deque arbeiten soll
  int Counter = 0;
  for (const auto &CurrVTable : Json["VTables"]) {
    Data.VTables.emplace_back(std::vector<std::string>());

    std::string NameOfCurrentVTable = std::to_string(Counter);

    for (const auto &CurrVFunc : CurrVTable) {
      Data.VTables[Counter].push_back(CurrVFunc);
    }

    Counter++;
  }

  return Data;
}

void DIBasedTypeHierarchyData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json JSON;

  for (const auto &Curr : NameToType) {
    JSON["NameToType"][Curr.first()] = Curr.second;
  }

  // for (const auto &Curr : TypeToVertex) {
  //   JSON["TypeToVertex"][Curr.first] = Curr.second;
  // }

  for (const auto &Curr : VertexTypes) {
    JSON["VertexTypes"].push_back(Curr);
  }

  for (const auto &Curr : TransitiveDerivedIndex) {
    JSON["TransitiveDerivedIndex"][Curr.first] = Curr.second;
  }

  for (const auto &Curr : Hierarchy) {
    JSON["Hierarchy"].push_back(Curr);
  }

  int Counter = 0;
  for (const auto &CurrVTable : VTables) {
    std::string NameForVTable = std::to_string(Counter);

    for (const auto &CurrVFunc : CurrVTable) {
      JSON["VTables"][NameForVTable].push_back(CurrVFunc);
    }

    Counter++;
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
