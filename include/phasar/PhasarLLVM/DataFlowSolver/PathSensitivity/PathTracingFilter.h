/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHTRACINGFILTER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHTRACINGFILTER_H

#include <type_traits>

namespace psr {
template <typename EndFilter, typename ErrFilter> struct PathTracingFilter {
  using end_filter_t = EndFilter;
  using err_filter_t = ErrFilter;
  EndFilter HasReachedEnd;
  ErrFilter IsErrorneousTransition;
};

namespace detail {
struct False2 {
  template <typename T, typename U>
  constexpr bool operator()(T && /*First*/, U && /*Second*/) const noexcept {
    return false;
  }
};
} // namespace detail

using DefaultPathTracingFilter =
    PathTracingFilter<detail::False2, detail::False2>;

template <typename F, typename Node, typename = void>
struct is_pathtracingfilter_for : std::false_type {};
template <typename F, typename Node>
struct is_pathtracingfilter_for<
    F, Node,
    std::enable_if_t<std::is_invocable_r_v<bool, typename F::end_filter_t,
                                           const Node *, const Node *> &&
                     std::is_invocable_r_v<bool, typename F::err_filter_t,
                                           const Node *, const Node *>>>
    : std::true_type {};

template <typename F, typename Node>
constexpr static bool is_pathtracingfilter_for_v =
    is_pathtracingfilter_for<F, Node>::value;
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHTRACINGFILTER_H
