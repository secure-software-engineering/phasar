/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_PATHEDGE_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_PATHEDGE_H

#include "phasar/Utils/ByRef.h"

#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace psr {

template <typename N, typename D> class PathEdge {

public:
  PathEdge(D DSource, N Target,
           D DTarget) noexcept(std::is_nothrow_move_constructible_v<N>
                                   &&std::is_nothrow_move_constructible_v<D>)
      : DSource(std::move(DSource)), Target(std::move(Target)),
        DTarget(std::move(DTarget)) {}

  [[nodiscard]] ByConstRef<D> factAtSource() const noexcept { return DSource; }

  [[nodiscard]] ByConstRef<N> getTarget() const noexcept { return Target; }

  [[nodiscard]] ByConstRef<D> factAtTarget() const noexcept { return DTarget; }

  [[nodiscard]] std::tuple<ByConstRef<D>, ByConstRef<N>, ByConstRef<D>>
  get() const noexcept {
    return {DSource, Target, DTarget};
  }

  [[nodiscard]] std::tuple<D, N, D> consume() noexcept {
    return {std::move(DSource), std::move(Target), std::move(DTarget)};
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const PathEdge &Edge) {
    return OS << "<" << Edge.DSource << "> -> <" << Edge.Target << ","
              << Edge.DTarget << ">";
  }

private:
  D DSource;
  N Target;
  D DTarget;
};

} // namespace psr

#endif
