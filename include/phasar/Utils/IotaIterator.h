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
    if constexpr (is_incrementable<T>) {
      ++Elem;
    } else {
      Elem = T(size_t(Elem) + 1);
    }
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

template <typename IdT>
using IotaRangeType = llvm::iterator_range<IotaIterator<IdT>>;

template <typename IdT>
[[nodiscard]] constexpr auto iota(IdT From, type_identity_t<IdT> To) noexcept {
  static_assert(is_explicitly_convertible_to<size_t, IdT> &&
                    is_explicitly_convertible_to<IdT, size_t>,
                "Iota only works on integers and integer-like types");
  using iterator_type = IotaIterator<std::decay_t<IdT>>;
  auto Ret = llvm::make_range(iterator_type(From), iterator_type(To));
  return Ret;
}

template <typename IdT> [[nodiscard]] constexpr auto iota(size_t To) noexcept {
  static_assert(is_explicitly_convertible_to<size_t, IdT> &&
                    is_explicitly_convertible_to<IdT, size_t>,
                "Iota only works on integers and integer-like types");
  using iterator_type = IotaIterator<std::decay_t<IdT>>;
  return llvm::make_range(iterator_type(), iterator_type(IdT(To)));
}

static_assert(is_iterable_over_v<IotaRangeType<int>, int>);

} // namespace psr

#endif // PHASAR_UTILS_IOTAITERATOR_H
