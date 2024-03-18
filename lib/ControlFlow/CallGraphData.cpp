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

#include "llvm/Support/ErrorHandling.h"

#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace psr {

void CallGraphData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json JSON;

  for (const auto &CurrentElement : FToFunctionVertexTy) {
    for (const auto &NTVal : CurrentElement.second) {
      JSON[CurrentElement.first].push_back(NTVal);
    }
  }

  OS << JSON;
}

void CallGraphData::deserializeJson(const llvm::Twine &Path) {
  nlohmann::json JSON = readJsonFile(Path);

  // map F to vector of n_t's
  for (const auto &CurrentFVal : JSON.get<nlohmann::json::object_t>()) {
    std::string FValueString = CurrentFVal.first;
    std::vector<int> FunctionVertexTyVals(CurrentFVal.second.size());

    for (const auto &CurrentFunctionVertexTy : CurrentFVal.second) {
      FunctionVertexTyVals.push_back(CurrentFunctionVertexTy);
    }

    FToFunctionVertexTy.insert({FValueString, FunctionVertexTyVals});
  }
}
} // namespace psr
