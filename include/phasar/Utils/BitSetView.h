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
#include <iterator>
#include <string>
#include <type_traits>

namespace psr {
namespace detail {

template <typename Derived, typename T> class BitSetViewMixin {
public:
  using value_type = T;
  using size_type = size_t;

  // NOLINTNEXTLINE(readability-identifier-naming)
  class iterator {
  public:
    using value_type = T;
    using reference = ByConstRef<T>;
    using pointer = T *;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    iterator &operator++() noexcept {
      ++It;
      return *this;
    }
    iterator operator++(int) noexcept {
      auto Ret = *this;
      ++*this;
      return Ret;
    }

    reference operator*() const noexcept { return (*Compr)[*It]; }

    [[nodiscard]] bool operator==(const iterator &Other) const noexcept {
      return It == Other.It;
    }
    [[nodiscard]] bool operator!=(const iterator &Other) const noexcept {
      return !(*this == Other);
    }

    explicit iterator(llvm::SmallBitVector::const_set_bits_iterator It,
                      const Compressor<T> *Compr) noexcept
        : It(It), Compr(Compr) {
      assert(Compr != nullptr);
    }

  private:
    llvm::SmallBitVector::const_set_bits_iterator It;
    const Compressor<T> *Compr{};
  };

  using const_iterator = iterator;

  [[nodiscard]] bool empty() const noexcept { return bits().empty(); }
  [[nodiscard]] size_t size() const noexcept { return bits().count(); }

  [[nodiscard]] size_t count(ByConstRef<T> Val) const noexcept {
    auto IdxOrNull = self().Compr->getOrNull(Val);
    return IdxOrNull && *IdxOrNull < bits().size() && bits().test(*IdxOrNull);
  }
  [[nodiscard]] bool contains(ByConstRef<T> Val) const noexcept {
    return count(Val);
  }

  [[nodiscard]] iterator begin() const noexcept {
    return iterator(bits().set_bits_begin(), self().Compr);
  }
  [[nodiscard]] iterator end() const noexcept {
    return iterator(bits().set_bits_end(), self().Compr);
  }

  [[nodiscard]] const auto &bits() const noexcept { return self().Bits; }

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

  static const llvm::SmallBitVector &
  assertNotNull(const llvm::SmallBitVector *Bits) noexcept {
    assert(Bits != nullptr);
    return *Bits;
  }

public:
  explicit BitSetView(const llvm::SmallBitVector *Bits,
                      const Compressor<T> *Compr) noexcept
      : Compr(Compr), Bits(assertNotNull(Bits)) {
    assert(Compr != nullptr);
  }

private:
  const Compressor<T> *Compr;
  const llvm::SmallBitVector &Bits;
};

template <typename T>
class OwningBitSetView
    : public detail::BitSetViewMixin<OwningBitSetView<T>, T> {
  friend detail::BitSetViewMixin<OwningBitSetView<T>, T>;

public:
  OwningBitSetView(Compressor<T> *Compr) noexcept : Compr(Compr) {
    assert(Compr != nullptr);
  }
  explicit OwningBitSetView(llvm::SmallBitVector Bits,
                            Compressor<T> *Compr) noexcept
      : Compr(Compr), Bits(std::move(Bits)) {
    assert(Compr != nullptr);
  }

  [[nodiscard]] BitSetView<T> view() const &noexcept {
    return BitSetView<T>(&Bits, Compr);
  }
  BitSetView<T> view() && = delete;

  operator BitSetView<T>() const &noexcept { return view(); }
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
      this->insert(std::forward<decltype(Elem)>(Elem));
    });
  }

  template <typename BSV>
  void insert(const detail::BitSetViewMixin<BSV, T> &Set) {
    Bits |= Set.bits();
  }

  void insert(const llvm::SmallBitVector &BV) { Bits |= BV; }

  template <typename SetT>
  [[nodiscard]] static OwningBitSetView create(SetT &&Set,
                                               Compressor<T> *Compr) {
    assert(Compr != nullptr);
    OwningBitSetView<T> Ret(Compr);
    Ret.insert(llvm::adl_begin(Set), llvm::adl_end(Set));
    return Ret;
  }

  [[nodiscard]] const llvm::SmallBitVector &bits() const &noexcept {
    return Bits;
  }
  [[nodiscard]] llvm::SmallBitVector &bits() &noexcept { return Bits; }
  [[nodiscard]] llvm::SmallBitVector bits() &&noexcept {
    return std::move(Bits);
  }

private:
  Compressor<T> *Compr{};
  llvm::SmallBitVector Bits{};
};

extern template class BitSetView<std::string>;
extern template class OwningBitSetView<std::string>;
} // namespace psr

#endif // PHASAR_UTILS_BITSETVIEW_H
