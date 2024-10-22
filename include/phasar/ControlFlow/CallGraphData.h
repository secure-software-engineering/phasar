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

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace psr {
struct CallGraphData {
  // Mangled FunName --> [CS-IDs]
  std::unordered_map<std::string, std::vector<uint32_t>> FToFunctionVertexTy{};

  CallGraphData() noexcept = default;
  void printAsJson(llvm::raw_ostream &OS);

  static CallGraphData deserializeJson(const llvm::Twine &Path);
  static CallGraphData loadJsonString(llvm::StringRef JsonAsString);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPHDATA_H
