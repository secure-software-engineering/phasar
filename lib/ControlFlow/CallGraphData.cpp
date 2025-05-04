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
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

namespace psr {
static CallGraphData getDataFromJson(const nlohmann::json &Json) {
  CallGraphData ToReturn;

  // map F to vector of n_t's
  for (const auto &[FVal, FunctionVertexTyVals] :
       Json.get<nlohmann::json::object_t>()) {
    auto &FValMappedVector = ToReturn.FToFunctionVertexTy[FVal];
    if (FunctionVertexTyVals.is_array()) {
      FValMappedVector = FunctionVertexTyVals.get<std::vector<uint32_t>>();
    } else if (!FunctionVertexTyVals.is_null()) {
      PHASAR_LOG_LEVEL(
          WARNING, "Invalid Function-CS IDs Array: " << FunctionVertexTyVals);
    }
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

  OS << JSON << '\n';
}

CallGraphData CallGraphData::deserializeJson(const llvm::Twine &Path) {
  return getDataFromJson(readJsonFile(Path));
}

CallGraphData CallGraphData::loadJsonString(llvm::StringRef JsonAsString) {
  auto ToStringify =
      nlohmann::json::parse(JsonAsString.begin(), JsonAsString.end());
  return getDataFromJson(ToStringify);
}

} // namespace psr
