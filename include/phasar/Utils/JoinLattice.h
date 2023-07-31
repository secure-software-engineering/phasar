/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AbstractJoinLattice.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_UTILS_JOINLATTICE_H
#define PHASAR_UTILS_JOINLATTICE_H

#include <type_traits>
#include <utility>

namespace psr {

template <typename L> struct JoinLatticeTraits {
  // static constexpr L top();
  // static constexpr L bottom();
  // static L join(L LHS, L RHS);
};

namespace detail {
template <typename L, typename = void>
struct HasJoinLatticeTraitsHelper : std::false_type {};
template <typename L>
struct HasJoinLatticeTraitsHelper<
    L, std::enable_if_t<
           std::is_convertible_v<decltype(JoinLatticeTraits<L>::top()), L> &&
           std::is_convertible_v<decltype(JoinLatticeTraits<L>::bottom()), L> &&
           std::is_convertible_v<decltype(JoinLatticeTraits<L>::join(
                                     std::declval<L>(), std::declval<L>())),
                                 L>>> : std::true_type {};
} // namespace detail
template <typename L>
static constexpr bool HasJoinLatticeTraits =
    detail::HasJoinLatticeTraitsHelper<L>::value;

template <typename AnalysisDomainTy, typename = void> class JoinLattice {
public:
  using l_t = typename AnalysisDomainTy::l_t;

  virtual ~JoinLattice() = default;
  virtual l_t topElement() = 0;
  virtual l_t bottomElement() = 0;
  virtual l_t join(l_t Lhs, l_t Rhs) = 0;
};

template <typename AnalysisDomainTy>
class JoinLattice<
    AnalysisDomainTy,
    std::enable_if_t<HasJoinLatticeTraits<typename AnalysisDomainTy::l_t>>> {
public:
  using l_t = typename AnalysisDomainTy::l_t;

  virtual ~JoinLattice() = default;
  virtual l_t topElement() { return JoinLatticeTraits<l_t>::top(); };
  virtual l_t bottomElement() { return JoinLatticeTraits<l_t>::bottom(); };
  virtual l_t join(l_t Lhs, l_t Rhs) {
    return JoinLatticeTraits<l_t>::join(std::move(Lhs), std::move(Rhs));
  };
};

template <typename L, typename = void> struct NonTopBotValue {
  using type = L;

  static L unwrap(L Value) noexcept(std::is_nothrow_move_constructible_v<L>) {
    return Value;
  }
};

} // namespace psr

#endif
