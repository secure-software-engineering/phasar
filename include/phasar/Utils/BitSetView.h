/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_BITSETVIEW_H
#define PHASAR_UTILS_BITSETVIEW_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"

#include <algorithm>
#include <cstddef>
#include <type_traits>

namespace psr {
namespace detail {
template <typename Derived, typename T> class BitSetViewMixin {
public:
  using value_type = T;
  using size_type = size_t;

  struct Transform {
    const Compressor<T> *Compr{};

    [[nodiscard]] ByConstRef<T> operator()(size_t Idx) const noexcept {
      return (*Compr)[Idx];
    }
  };

  using iterator =
      llvm::mapped_iterator<llvm::SmallBitVector::const_set_bits_iterator,
                            Transform>;
  using const_iterator = iterator;

  [[nodiscard]] bool empty() const noexcept { return self().Bits.empty(); }
  [[nodiscard]] size_t size() const noexcept { return self().Bits.count(); }

  [[nodiscard]] size_t count(ByConstRef<T> Val) const noexcept {
    auto IdxOrNull = self().Compr->getOrNull(Val);
    return IdxOrNull && *IdxOrNull < self().Bits.size() &&
           self().Bits.test(*IdxOrNull);
  }
  [[nodiscard]] bool contains(ByConstRef<T> Val) const noexcept {
    return count(Val);
  }

  [[nodiscard]] iterator begin() const noexcept {
    return llvm::map_iterator(self().Bits.begin(), Transform{self().Compr});
  }
  [[nodiscard]] iterator end() const noexcept {
    return llvm::map_iterator(self().Bits.end(), Transform{self().Compr});
  }

private:
  [[nodiscard]] Derived &self() noexcept {
    static_assert(std::is_base_of_v<BitSetViewMixin, Derived>);
    return static_cast<Derived &>(*this);
  }
  [[nodiscard]] const Derived &self() const noexcept {
    static_assert(std::is_base_of_v<BitSetViewMixin, Derived>);
    return static_cast<const Derived &>(*this);
  }
};
} // namespace detail

template <typename T>
class BitSetView : public detail::BitSetViewMixin<BitSetView<T>, T> {
  friend detail::BitSetViewMixin<BitSetView<T>, T>;

  static llvm::SmallBitVector &
  assertNotNull(llvm::SmallBitVector *Bits) noexcept {
    assert(Bits != nullptr);
    return *Bits;
  }

public:
  BitSetView(llvm::SmallBitVector *Bits, Compressor<T> *Compr) noexcept
      : Bits(assertNotNull(Bits)), Compr(Compr) {
    assert(Compr != nullptr);
  }

private:
  llvm::SmallBitVector &Bits;
  Compressor<T> *Compr;
};

template <typename T>
class OwningBitSetView
    : public detail::BitSetViewMixin<OwningBitSetView<T>, T> {
  friend detail::BitSetViewMixin<OwningBitSetView<T>, T>;

public:
  explicit OwningBitSetView(llvm::SmallBitVector Bits,
                            Compressor<T> *Compr) noexcept
      : Bits(std::move(Bits)), Compr(Compr) {
    assert(Compr != nullptr);
  }

  operator BitSetView<T>() const &noexcept {
    return BitSetView<T>(&Bits, Compr);
  }
  operator BitSetView<T>() && = delete;

  void insert(T Val) noexcept {
    auto Idx = Compr->getOrInsert(std::move(Val));
    if (Bits.size() <= Idx) {
      Bits.resize(Idx + 1);
    }
    Bits.set(Idx);
  }

  template <typename Iter> void insert(Iter Begin, Iter End) {
    std::for_each(Begin, End, [this](auto &&Elem) {
      insert(std::forward<decltype(Elem)>(Elem));
    });
  }

private:
  llvm::SmallBitVector Bits;
  Compressor<T> *Compr;
};
} // namespace psr

#endif // PHASAR_UTILS_BITSETVIEW_H
