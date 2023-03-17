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

  [[no_unique_address]] end_filter_t HasReachedEnd;
  [[no_unique_address]] err_filter_t IsErrorneousTransition;
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

template <typename F, typename NodeRef, typename = void>
struct is_pathtracingfilter_for : std::false_type {};

template <typename EndFilter, typename ErrFilter, typename NodeRef>
struct is_pathtracingfilter_for<
    PathTracingFilter<EndFilter, ErrFilter>, NodeRef,
    std::enable_if_t<std::is_invocable_r_v<bool, EndFilter, NodeRef, NodeRef> &&
                     std::is_invocable_r_v<bool, ErrFilter, NodeRef, NodeRef>>>
    : std::true_type {};

template <typename F, typename NodeRef>
constexpr static bool is_pathtracingfilter_for_v =
    is_pathtracingfilter_for<F, NodeRef>::value;
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_PATHSENSITIVITY_PATHTRACINGFILTER_H
