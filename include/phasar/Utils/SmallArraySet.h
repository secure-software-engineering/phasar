#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Compiler.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <numeric>
#include <type_traits>

namespace psr {

/// \brief Simple set container similar to the upcoming std::flat_set but only
/// uses  sorting+binary-search, once the size exceeds a fixed threshold. Below
/// the threshold unique-insertion + lookup is handled via linear search.
///
/// Should work well, if the expected size is small.
/// Use the static methods fromSorted() and fromSortedUniqued(), to speedup
/// construction,. if you can make some assumptions about the incoming data.
template <typename T,
          unsigned N =
              llvm::CalculateSmallVectorDefaultInlinedElements<T>::value>
class SmallArraySet : private llvm::SmallVector<T, N> {
public:
  using typename llvm::SmallVector<T, N>::value_type;
  using typename llvm::SmallVector<T, N>::iterator;
  using typename llvm::SmallVector<T, N>::const_iterator;
  using typename llvm::SmallVector<T, N>::const_reference;
  using typename llvm::SmallVector<T, N>::reference;
  using typename llvm::SmallVector<T, N>::const_pointer;
  using typename llvm::SmallVector<T, N>::pointer;
  using typename llvm::SmallVector<T, N>::difference_type;

  static constexpr size_t LinearThreshold = std::max<size_t>(1, 64 / sizeof(T));

  SmallArraySet() noexcept = default;
  SmallArraySet(std::initializer_list<T> IList)
      : llvm::SmallVector<T, N>(IList) {
    psr::sortUnique(base());
  }

  explicit SmallArraySet(llvm::ArrayRef<T> Elems)
      : llvm::SmallVector<T, N>(Elems) {
    psr::sortUnique(base());
  }

  [[nodiscard]] static SmallArraySet
  fromSorted(llvm::SmallVectorImpl<T> &&Vec) {
    assert(std::ranges::is_sorted(Vec) && "Vec is not sorted");
    SmallArraySet Ret(std::move(Vec), /*PreSortedAndUniqued=*/std::true_type{});

    Ret.base().erase(std::unique(begin(), end()), end());

    return Ret;
  }

  [[nodiscard]] static SmallArraySet
  fromSortedUniqued(llvm::SmallVectorImpl<T> &&Vec) {
    assert(std::ranges::is_sorted(Vec) && "Vec is not sorted");
    assert(std::ranges::adjacent_find(Vec) == Vec.end() &&
           "Vec is not uniqued");

    SmallArraySet Ret(std::move(Vec), /*PreSortedAndUniqued=*/std::true_type{});
    return Ret;
  }

  // Note: default the special member functions to explicitly make the move ctor
  // noexcept
  SmallArraySet(const SmallArraySet &) = default;
  SmallArraySet(SmallArraySet &&) noexcept = default;
  SmallArraySet &operator=(const SmallArraySet &) = default;
  SmallArraySet &operator=(SmallArraySet &&) noexcept = default;
  ~SmallArraySet() = default;

  [[nodiscard]] operator llvm::ArrayRef<T>() const noexcept { return base(); }

  using llvm::SmallVector<T, N>::begin;
  using llvm::SmallVector<T, N>::end;
  using llvm::SmallVector<T, N>::size;
  using llvm::SmallVector<T, N>::empty;
  using llvm::SmallVector<T, N>::reserve;
  using llvm::SmallVector<T, N>::operator==;
  using llvm::SmallVector<T, N>::operator!=;

  // Without inline, clang may not inline this, even in O3
  // NOLINTNEXTLINE(readability-redundant-inline-specifier)
  inline bool insert(ByConstRef<T> Val) {
    const auto Size = this->size();
    const auto Begin = this->begin();
    const auto End = this->end();

    if (Size < LinearThreshold) [[likely]] {
      if (std::find(Begin, End, Val) != End) {
        return false;
      }

      this->push_back(Val);

      if (Size + 1 == LinearThreshold) [[unlikely]] {
        std::ranges::sort(base());
      }

      return true;
    }

    return insertImpl(Val);
  }

  // Assume, the new Val is not in the set yet
  void insertUnique(T Val) {
    assert(!contains(Val));
    if (this->size() < LinearThreshold || Val > this->back()) {
      this->push_back(std::move(Val));
      return;
    }
    insertImpl(Val);
  }

  void insert(const SmallArraySet &Other) {
    const auto Size = size();
    const auto OtherSize = Other.size();

    llvm::ArrayRef<T> OtherArr = Other;

    if (OtherSize <= Size && Size + OtherSize <= LinearThreshold) {
      size_t I = 0;
      for (; I != OtherSize; ++I) {
        const auto End = this->end();
        ByConstRef<T> Val = OtherArr[I];
        if (std::find(this->begin(), End, Val) != End) {
          continue;
        }
        if (this->size() == LinearThreshold) {
          break;
        }
        this->push_back(Val);
      }
      if (I == OtherSize) {
        if (this->size() == LinearThreshold) [[unlikely]] {
          std::ranges::sort(base());
        }
        return;
      }

      OtherArr = OtherArr.slice(I);
    }

    insertImpl(OtherArr);
  }

  void insertAll(auto &&Range) {
    size_t EstimatedRngSize = SIZE_MAX;
    if constexpr (requires() {
                    { Range.size() } -> std::convertible_to<size_t>;
                  }) {
      EstimatedRngSize = Range.size();
    }

    if (size() < LinearThreshold && EstimatedRngSize < LinearThreshold) {
      for (auto &&Elem : Range) {
        insert(PSR_FWD(Elem));
      }
      return;
    }

    this->append(llvm::adl_begin(Range), llvm::adl_end(Range));
    psr::sortUnique(base());
  }

  [[nodiscard]] SmallArraySet setUnion(const SmallArraySet &Other) const {

    const auto Size = size();
    const auto OtherSize = Other.size();
    if (std::min(Size, OtherSize) < LinearThreshold) {
      const auto ThisSmaller = Size < OtherSize;
      const auto &Smaller = ThisSmaller ? *this : Other;
      const auto &Larger = ThisSmaller ? Other : *this;
      auto Merged = Larger;
      Merged.insert(Smaller);
      return Merged;
    }

    SmallArraySet Merged{};
    Merged.resize_for_overwrite(size() + Other.size());
    auto LastOut = std::set_union(begin(), end(), Other.begin(), Other.end(),
                                  Merged.begin());
    Merged.base().erase(LastOut, Merged.end());
    return Merged;
  }

  bool intersectWith(const SmallArraySet &Other) {
    const auto Size = size();
    const auto OtherSize = Other.size();

    if (Size < LinearThreshold || OtherSize > Size) {
      auto It = std::ranges::remove_if(
          base(), [&Other](const auto &Elem) { return !Other.contains(Elem); });
      base().erase(It.begin(), It.end());
      return Size != size();
    }

    // TODO: Optimize

    SmallArraySet Merged{};
    Merged.resize_for_overwrite(std::min(Size, OtherSize));

    auto [Unused1, Unused2, LastOut] =
        std::ranges::set_intersection(*this, Other, Merged.begin());
    Merged.base().erase(LastOut, Merged.end());
    *this = std::move(Merged);
    return Size != size();
  }

  [[nodiscard]] SmallArraySet
  setIntersection(const SmallArraySet &Other) const {
    const auto Size = size();
    const auto OtherSize = Other.size();

    if (std::min(Size, OtherSize) < LinearThreshold) {
      auto Ret = Size < OtherSize ? *this : Other;
      const auto &Larger = Size < OtherSize ? Other : *this;
      auto It = std::ranges::remove_if(
          Ret, [&Larger](const auto &Elem) { return !Larger.contains(Elem); });
      Ret.base().erase(It.begin(), It.end());
      return Ret;
    }

    SmallArraySet Merged{};
    Merged.resize_for_overwrite(std::min(Size, OtherSize));

    auto [Unused1, Unused2, LastOut] =
        std::ranges::set_intersection(*this, Other, Merged.begin());
    Merged.base().erase(LastOut, Merged.end());
    return Merged;
  }

  [[nodiscard]] bool contains(ByConstRef<T> Val) const noexcept {
    if (this->size() < LinearThreshold) {
      return llvm::is_contained(base(), Val);
    }

    return std::ranges::binary_search(base(), Val);
  }

  void sort() {
    if (size() >= LinearThreshold) {
      // Always sorted
      return;
    }

    std::ranges::sort(base());
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE auto foreach (
      std::invocable<const T &> auto Handler) const {
    return psr::foreachInRange(base(), std::move(Handler));
  }

  [[nodiscard]] friend auto hash_value(const SmallArraySet &Set) noexcept {
    if (Set.size() < LinearThreshold) {
      if constexpr (std::is_trivially_copyable_v<T>) {
        T Arr[LinearThreshold]{};
        memcpy(&Arr, Set.data(), Set.size_in_bytes());
        std::ranges::sort(Arr, Arr + Set.size());
        return llvm::hash_combine_range(Arr, Arr + Set.size());
      } else {
        // Some reduction that ignores the order
        return std::transform_reduce(Set.begin(), Set.end(), 37,
                                     std::bit_xor<>{}, [](ByConstRef<T> Val) {
                                       using llvm::hash_value;
                                       return hash_value(Val);
                                     });
      }
    } else {
      return llvm::hash_combine_range(Set.begin(), Set.end());
    }
  }

  [[nodiscard]] bool operator==(const SmallArraySet &Other) const noexcept {
    if (size() != Other.size()) {
      return false;
    }

    if (size() < LinearThreshold) {
      if (hash_value(*this) != hash_value(Other)) {
        // Some pre-check to avoid the quadratic loop.
        // XXX: Need to measure, whether this actually helps
        return false;
      }

      for (const auto &Val : Other) {
        if (!contains(Val)) {
          return false;
        }
      }
      return true;
    }

    return std::equal(begin(), end(), Other.begin());
  }

private:
  explicit SmallArraySet(llvm::SmallVectorImpl<T> &&Elems,
                         std::true_type /*PreSortedAndUniqued*/)
      : llvm::SmallVector<T, N>(std::move(Elems)) {}

  bool insertImpl(ByConstRef<T> Val) {
    const auto It = std::ranges::lower_bound(base(), Val);
    if (It != this->end() && *It == Val) {
      return false;
    }

    base().insert(It, Val);
    return true;
  }

  void insertImpl(llvm::ArrayRef<T> OtherArr) {
    if (OtherArr.size() < 3) {
      // For small sizes, it is probably better to insert the elements
      // one-by-one, instead of using std::sort.
      // TODO: Find good threshold

      for (const auto &Val : OtherArr) {
        insert(Val);
      }
      return;
    }

    this->append(OtherArr.begin(), OtherArr.end());
    psr::sortUnique(base());
  }

  [[nodiscard]] constexpr auto &base() noexcept {
    return static_cast<llvm::SmallVectorImpl<T> &>(*this);
  }
  [[nodiscard]] constexpr const auto &base() const noexcept {
    return static_cast<const llvm::SmallVectorImpl<T> &>(*this);
  }
};
} // namespace psr
