/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Sriteja Kummita and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_PATHSENSITIVITY_DEFAULTFLOWPATH_H
#define PHASAR_PHASARLLVM_DATAFLOW_PATHSENSITIVITY_DEFAULTFLOWPATH_H

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {
template <typename N> struct DefaultFlowPath {
  llvm::SmallVector<N, 0> Path;

  DefaultFlowPath(llvm::ArrayRef<N> Path) : Path(Path.begin(), Path.end()) {}

  [[nodiscard]] auto begin() noexcept { return Path.begin(); }
  [[nodiscard]] auto end() noexcept { return Path.end(); }
  [[nodiscard]] auto begin() const noexcept { return Path.begin(); }
  [[nodiscard]] auto end() const noexcept { return Path.end(); }
  [[nodiscard]] auto cbegin() const noexcept { return Path.cbegin(); }
  [[nodiscard]] auto cend() const noexcept { return Path.cend(); }

  [[nodiscard]] size_t size() const noexcept { return Path.size(); }
  [[nodiscard]] bool empty() const noexcept { return Path.empty(); }

  [[nodiscard]] decltype(auto) operator[](size_t Idx) const {
    return Path[Idx];
  }

  [[nodiscard]] operator llvm::ArrayRef<N>() const noexcept { return Path; }

  [[nodiscard]] bool operator==(const DefaultFlowPath &Other) const noexcept {
    return Other.Path == Path;
  }
  [[nodiscard]] bool operator!=(const DefaultFlowPath &Other) const noexcept {
    return !(*this == Other);
  }

  void print(llvm::raw_ostream &ROS) const {
    ROS << ">>>>>> ";
    for (const auto &P : Path) {
      ROS << llvmIRToString(P) << '\n';
    }
    ROS << ">>>>>>" << '\n';
  }
};

template <typename N>
using DefaultFlowPathSequence = std::vector<DefaultFlowPath<N>>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_PATHSENSITIVITY_FLOWPATH_H
