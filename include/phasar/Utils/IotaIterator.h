/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_IOTAITERATOR_H
#define PHASAR_UTILS_IOTAITERATOR_H

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/iterator_range.h"

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace psr {
/// An iterator that iterates over the same value a specified number of times
template <typename T> class IotaIterator {
public:
  using value_type = T;
  using reference = T;
  using pointer = const T *;
  using difference_type = ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  constexpr reference operator*() const noexcept { return Elem; }
  constexpr pointer operator->() const noexcept { return &Elem; }

  constexpr IotaIterator &operator++() noexcept {
    ++Elem;
    return *this;
  }
  constexpr IotaIterator operator++(int) noexcept {
    auto Ret = *this;
    ++*this;
    return Ret;
  }

  constexpr bool operator==(const IotaIterator &Other) const noexcept {
    return Other.Elem == Elem;
  }
  constexpr bool operator!=(const IotaIterator &Other) const noexcept {
    return !(*this == Other);
  }

  constexpr explicit IotaIterator(T Elem) noexcept : Elem(Elem) {}

  constexpr IotaIterator() noexcept = default;

private:
  T Elem{};
};

template <typename T>
using IotaRangeType = llvm::iterator_range<IotaIterator<T>>;
template <typename T>
constexpr auto iota(T From, type_identity_t<T> To) noexcept {
  static_assert(std::is_integral_v<T>, "Iota only works on integers");
  using iterator_type = IotaIterator<std::decay_t<T>>;
  auto Ret = llvm::make_range(iterator_type(From), iterator_type(To));
  return Ret;
}

static_assert(is_iterable_over_v<IotaRangeType<int>, int>);

} // namespace psr

#endif // PHASAR_UTILS_IOTAITERATOR_H
