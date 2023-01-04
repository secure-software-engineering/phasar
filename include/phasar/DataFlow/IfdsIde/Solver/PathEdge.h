/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_PATHEDGE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_PATHEDGE_H

#include "llvm/Support/raw_ostream.h"
#include <type_traits>

namespace psr {

template <typename N, typename D> class PathEdge {

public:
  PathEdge(D DSource, N Target, D DTarget) noexcept
      : Target(std::move(Target)), DSource(std::move(DSource)),
        DTarget(std::move(DTarget)) {}

  ~PathEdge() = default;

  PathEdge(const PathEdge &) noexcept(
      std::is_nothrow_copy_constructible_v<N>
          &&std::is_nothrow_copy_constructible_v<D>) = default;

  PathEdge &operator=(const PathEdge &) noexcept(
      std::is_nothrow_copy_assignable_v<N>
          &&std::is_nothrow_copy_assignable_v<D>) = default;

  PathEdge(PathEdge &&) noexcept = default;

  PathEdge &operator=(PathEdge &&) noexcept = default;

  [[nodiscard]] N getTarget() const { return Target; }

  [[nodiscard]] D factAtSource() const { return DSource; }

  [[nodiscard]] D factAtTarget() const { return DTarget; }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const PathEdge &Edge) {
    return OS << "<" << Edge.DSource << "> -> <" << Edge.Target << ","
              << Edge.DTarget << ">";
  }

private:
  N Target;
  D DSource;
  D DTarget;
};

} // namespace psr

#endif
