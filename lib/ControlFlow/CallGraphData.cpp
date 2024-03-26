/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraphData.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/NlohmannLogging.h"

#include <nlohmann/json_fwd.hpp>

namespace psr {
static CallGraphData getDataFromJson(const nlohmann::json &Json) {
  CallGraphData ToReturn;

  // map F to vector of n_t's
  for (const auto &CurrentFVal : Json.get<nlohmann::json::object_t>()) {
    std::string FValueString = CurrentFVal.first;
    std::vector<int> FunctionVertexTyVals;
    FunctionVertexTyVals.reserve(CurrentFVal.second.size());

    for (const auto &CurrentFunctionVertexTy : CurrentFVal.second) {
      FunctionVertexTyVals.push_back(CurrentFunctionVertexTy);
    }

    ToReturn.FToFunctionVertexTy.try_emplace(FValueString,
                                             FunctionVertexTyVals);
  }

  return ToReturn;
}

void CallGraphData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json JSON;

  for (const auto &[Fun, Callers] : FToFunctionVertexTy) {
    auto &JCallers = JSON[Fun];

    for (const auto &CS : Callers) {
      JCallers.push_back(CS);
    }
  }

  OS << JSON;
}

CallGraphData CallGraphData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

CallGraphData CallGraphData::loadJsonString(const std::string &JsonAsString) {
  // nlohmann::json::parse needs a std::string, llvm::Twine won't work
  nlohmann::json ToStringify = nlohmann::json::parse(JsonAsString);
  return getDataFromJson(ToStringify);
}

} // namespace psr
