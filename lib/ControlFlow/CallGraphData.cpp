/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraphData.h"

#include "phasar/Utils/NlohmannLogging.h"
#include "llvm/Support/ErrorHandling.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json_fwd.hpp>

namespace psr {
void CallGraphData::printAsJson(llvm::raw_ostream &OS) {
  nlohmann::json JSON;

  for (const auto &CurrentElement : FToFunctionVertexTy) {
    for (const auto &NTValString : CurrentElement.second) {
      JSON[CurrentElement.first].push_back(NTValString);
    }
  }

  OS << JSON;
}

void CallGraphData::deserializeJson(const llvm::Twine &Path) {
  std::ifstream IFS(Path.str());
  std::string Data((std::istreambuf_iterator<char>(IFS)),
                       (std::istreambuf_iterator<char>()));
  nlohmann::json JSON = nlohmann::json::parse(Data);

  if (!JSON.is_object()) {
    llvm::report_fatal_error("Invalid Json: not an object!");
  }
  // map F to vector of n_t's

  for (const auto &CurrentFVal : JSON.get<nlohmann::json::object_t>()) {
    std::string FValueString = CurrentFVal.first;
    std::vector<std::string> FunctionVertexTyStrings(CurrentFVal.second.size());
    for (const auto &CurrentFunctionVertexTy : CurrentFVal.second) {
      FunctionVertexTyStrings.push_back(CurrentFunctionVertexTy);
    }

    FToFunctionVertexTy.insert({FValueString, FunctionVertexTyStrings});
  }
}
} // namespace psr
