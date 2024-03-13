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
struct CallersOfData {
  CallersOfData() noexcept = default;
  std::unordered_map<std::string, std::vector<std::string>> FToFunctionVertexTy{};
};

struct CallGraphData {
  CallGraphData() noexcept = default;
  std::vector<CallersOfData> CallersOf{};
  void printAsJson(llvm::raw_ostream &OS);
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_CALLGRAPHDATA_H
