/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_PATHSENSITIVITY_FLOWPATH_H
#define PHASAR_PHASARLLVM_DATAFLOW_PATHSENSITIVITY_FLOWPATH_H

#include "z3++.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

namespace psr {
template <typename N> struct FlowPath {
  llvm::SmallVector<N, 0> Path;
  z3::expr Constraint;
  z3::model Model;

  FlowPath(llvm::ArrayRef<N> Path, const z3::expr &Constraint)
      : Path(Path.begin(), Path.end()), Constraint(Constraint),
        Model(Constraint.ctx()) {}
  FlowPath(llvm::ArrayRef<N> Path, const z3::expr &Constraint,
           const z3::model &Model)
      : Path(Path.begin(), Path.end()), Constraint(Constraint), Model(Model) {}

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

  [[nodiscard]] bool operator==(const FlowPath &Other) const noexcept {
    return Other.Path == Path;
  }
  [[nodiscard]] bool operator!=(const FlowPath &Other) const noexcept {
    return !(*this == Other);
  }
};

template <typename N> using FlowPathSequence = std::vector<FlowPath<N>>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_PATHSENSITIVITY_FLOWPATH_H
