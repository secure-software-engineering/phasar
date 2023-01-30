/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PathEdge.h
 *
 *  Created on: 16.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_PATHEDGE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_PATHEDGE_H

#include "phasar/PhasarLLVM/Utils/ByRef.h"

#include "llvm/Support/raw_ostream.h"

#include <ostream>
#include <type_traits>

namespace psr {

template <typename N, typename D> class PathEdge {
private:
  N Target;
  D DSource;
  D DTarget;

public:
  PathEdge(D DSource, N Target,
           D DTarget) noexcept(std::is_nothrow_move_constructible_v<N>
                                   &&std::is_nothrow_move_constructible_v<D>)
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

  [[nodiscard]] ByConstRef<N> getTarget() const noexcept { return Target; }

  [[nodiscard]] ByConstRef<D> factAtSource() const noexcept { return DSource; }

  [[nodiscard]] ByConstRef<D> factAtTarget() const noexcept { return DTarget; }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const PathEdge &Edge) {
    return OS << "<" << Edge.DSource << "> -> <" << Edge.Target << ","
              << Edge.DTarget << ">";
  }
};

} // namespace psr

#endif
