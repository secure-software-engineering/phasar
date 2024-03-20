/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPHDATA_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPHDATA_H

#include "llvm/Support/raw_ostream.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace psr {
struct CallGraphData {
  CallGraphData() noexcept = default;
  std::unordered_map<std::string, std::vector<int>> FToFunctionVertexTy{};
  void printAsJson(llvm::raw_ostream &OS);
  static CallGraphData deserializeJson(const llvm::Twine &Path);
  static CallGraphData loadJsonString(const std::string &JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPHDATA_H
