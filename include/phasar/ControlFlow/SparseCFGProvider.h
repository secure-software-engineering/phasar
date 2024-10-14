/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLFLOW_SPARSECFGPROVIDER_H
#define PHASAR_CONTROLFLOW_SPARSECFGPROVIDER_H

#include "phasar/Utils/ByRef.h"

#include <type_traits>

namespace psr {
template <typename T> T valueOf(T Val) { return Val; }

template <typename Derived, typename F, typename V> class SparseCFGProvider {
public:
  using f_t = F;
  using v_t = V;

  template <typename D>
  [[nodiscard]] decltype(auto) getSparseCFG(ByConstRef<f_t> Fun,
                                            const D &Val) const {
    using psr::valueOf;
    static_assert(std::is_convertible_v<decltype(valueOf(Val)), v_t>);
    return self().getSparseCFGImpl(Fun, valueOf(Val));
  }

private:
  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }
};

template <typename T, typename D, typename = void>
struct has_getSparseCFG : std::false_type {}; // NOLINT
template <typename T, typename D>
struct has_getSparseCFG<
    T, D,
    std::void_t<decltype(std::declval<const T>().getSparseCFG(
        std::declval<typename T::f_t>(), std::declval<D>()))>>
    : std::true_type {};

template <typename T, typename D>
// NOLINTNEXTLINE
static constexpr bool has_getSparseCFG_v = has_getSparseCFG<T, D>::value;
} // namespace psr

#endif // PHASAR_CONTROLFLOW_SPARSECFGPROVIDER_H
