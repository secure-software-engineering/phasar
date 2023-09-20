/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLFLOW_CALLGRAPHBASE_H
#define PHASAR_CONTROLFLOW_CALLGRAPHBASE_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include "nlohmann/json.hpp"

namespace psr {
template <typename T> struct CGTraits {
  // using n_t
  // using f_t
};

/// Base class of all CallGraph implementations within phasar (currently only
/// CallGraph<N, F>).
/// Only represents the data, not how to create it.
template <typename Derived> class CallGraphBase {
public:
  using n_t = typename CGTraits<Derived>::n_t;
  using f_t = typename CGTraits<Derived>::f_t;

  /// Returns an iterable range of all possible callee candidates at the given
  /// call-site induced by the used call-graph.
  ///
  /// NOTE: This function is typically called in a hot part of the analysis and
  /// should therefore be very fast
  [[nodiscard]] decltype(auto) getCalleesOfCallAt(ByConstRef<n_t> Inst) const
      noexcept(noexcept(self().getCalleesOfCallAtImpl(Inst))) {
    static_assert(
        is_iterable_over_v<decltype(self().getCalleesOfCallAtImpl(Inst)), f_t>);
    return self().getCalleesOfCallAtImpl(Inst);
  }

  /// Returns an iterable range of all possible call-site candidates that may
  /// call the given function induced by the used call-graph.
  [[nodiscard]] decltype(auto) getCallersOf(ByConstRef<f_t> Fun) const {
    static_assert(
        is_iterable_over_v<decltype(self().getCallersOfImpl(Fun)), n_t>);
    return self().getCallersOfImpl(Fun);
  }

private:
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};
} // namespace psr

#endif // PHASAR_CONTROLFLOW_CALLGRAPHBASE_H
