/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_SCOPEEXIT_H
#define PHASAR_UTILS_SCOPEEXIT_H

#include <type_traits>
#include <utility>

namespace psr {
/// See "https://en.cppreference.com/w/cpp/experimental/scope_exit/scope_exit"
template <typename Fn> class scope_exit { // NOLINT
public:
  template <typename FFn, typename = decltype(std::declval<FFn>()())>
  scope_exit(FFn &&F) noexcept(std::is_nothrow_constructible_v<Fn, FFn &&>)
      : F(std::forward<FFn>(F)) {}

  ~scope_exit() noexcept { F(); }

  scope_exit(const scope_exit &) = delete;
  scope_exit(scope_exit &&) = delete;

  scope_exit &operator=(const scope_exit &) = delete;
  scope_exit &operator=(scope_exit &&) = delete;

private:
  Fn F;
};

template <typename Fn> scope_exit(Fn) -> scope_exit<Fn>;

} // namespace psr

#endif // PHASAR_UTILS_SCOPEEXIT_H
