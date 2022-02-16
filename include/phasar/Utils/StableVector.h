/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_STABLEVECTOR_H_
#define PHASAR_UTILS_STABLEVECTOR_H_

#include <cstddef>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <type_traits>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

namespace psr {
template <typename T, bool DeallocateElements = true> class StableVector;
// NOLINTBEGIN(readability-identifier-naming)

/// Common base class of StableVector<T, true> and StableVector<T, false>.
/// Should be used for genericly passing const-ref arguments
template <typename T> class StableVectorBase {
public:
  void push_back(const T &Elem) { emplace_back(Elem); }
  void push_back(T &&Elem) { emplace_back(std::move(Elem)); }

  template <typename... ArgTys> T &emplace_back(ArgTys &&...Args) {
    auto Ret = new (Alloc.allocate(1)) T(std::forward<ArgTys>(Args)...);
    Data.push_back(Ret);
    return *Ret;
  }

  void reserve(size_t Capacity) { Data.reserve(Capacity); }

  [[nodiscard]] T &back() noexcept { return *Data.back(); }
  [[nodiscard]] const T &back() const noexcept { return *Data.back(); }

  T &operator[](size_t Idx) noexcept { return *Data[Idx]; }
  const T &operator[](size_t Idx) const noexcept { return *Data[Idx]; }

  [[nodiscard]] bool empty() const noexcept { return Data.empty(); }
  [[nodiscard]] size_t size() const noexcept { return Data.size(); }
  [[nodiscard]] size_t capacity() const noexcept { return Data.capacity(); }

  template <bool IsConst> class Iterator {
  public:
    using value_type = std::conditional_t<IsConst, const T, T>;
    using reference = value_type &;
    using pointer = value_type *;
    using difference_type = ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    Iterator &operator++() noexcept {
      ++Ptr;
      return *this;
    }

    Iterator operator++(int) noexcept {
      auto Ret = *this;
      ++*this;
      return Ret;
    }

    Iterator &operator--() noexcept {
      --Ptr;
      return *this;
    }

    Iterator operator--(int) noexcept {
      auto Ret = *this;
      --*this;
      return Ret;
    }

    Iterator &operator+=(difference_type N) noexcept {
      Ptr += N;
      return *this;
    }

    Iterator &operator-=(difference_type N) noexcept {
      Ptr -= N;
      return *this;
    }

    friend Iterator operator+(Iterator LHS, difference_type RHS) noexcept {
      return LHS.Ptr + RHS;
    }
    friend Iterator operator+(difference_type LHS, Iterator RHS) noexcept {
      return RHS + LHS;
    }
    friend Iterator operator-(Iterator LHS, difference_type RHS) noexcept {
      return LHS.Ptr - RHS;
    }

    friend difference_type operator-(Iterator LHS, Iterator RHS) noexcept {
      return LHS.Ptr - RHS.Ptr;
    }

    reference operator*() noexcept { return **Ptr; }
    pointer operator->() noexcept { return *Ptr; }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    reference operator[](difference_type Idx) noexcept { return *(Ptr[Idx]); }

    friend bool operator==(Iterator LHS, Iterator RHS) noexcept {
      return LHS.Ptr == RHS.Ptr;
    }
    friend bool operator!=(Iterator LHS, Iterator RHS) noexcept {
      return !(LHS == RHS);
    }
    friend bool operator<(Iterator LHS, Iterator RHS) noexcept {
      return LHS.Ptr < RHS.Ptr;
    }

    friend bool operator>(Iterator LHS, Iterator RHS) noexcept {
      return RHS < LHS;
    }
    friend bool operator<=(Iterator LHS, Iterator RHS) noexcept {
      return !(LHS > RHS);
    }
    friend bool operator>=(Iterator LHS, Iterator RHS) noexcept {
      return !(LHS < RHS);
    }

    Iterator(const Iterator &) noexcept = default;
    template <bool C, typename = std::enable_if_t<IsConst && !C>>
    Iterator(Iterator<C> Other) noexcept : Ptr(Other.Ptr) {}

    ~Iterator() = default;
    Iterator &operator=(const Iterator &) noexcept = default;

  private:
    friend class StableVectorBase;
    Iterator(T *const *Ptr) noexcept : Ptr(Ptr) {}

    T *const *Ptr = nullptr;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using value_type = T;

  iterator begin() noexcept { return Data.data(); }
  iterator end() noexcept { return Data.data() + Data.size(); }

  const_iterator cbegin() const noexcept { return Data.data(); }
  const_iterator cend() const noexcept { return Data.data() + Data.size(); }

  const_iterator begin() const noexcept { return cbegin(); }
  const_iterator end() const noexcept { return cend(); }

  /// Don't copy objects of the base-class!
  StableVectorBase(const StableVectorBase &) = delete;
  StableVectorBase &operator=(const StableVectorBase &) = delete;
  StableVectorBase &operator=(StableVectorBase &&) = delete;
  ~StableVectorBase() = default;

private:
  friend class StableVector<T, true>;
  friend class StableVector<T, false>;

  StableVectorBase(std::pmr::polymorphic_allocator<T> Alloc) noexcept
      : Alloc(std::move(Alloc)) {}
  StableVectorBase(llvm::SmallVector<T *, 0> Data,
                   std::pmr::polymorphic_allocator<T> Alloc) noexcept
      : Data(std::move(Data)), Alloc(std::move(Alloc)) {}

  StableVectorBase(StableVectorBase &&) noexcept = default;

  llvm::SmallVector<T *, 0> Data;
  std::pmr::polymorphic_allocator<T> Alloc;
};

/// A vector, where references to elements remain stable after reallocations.
/// Iterators, however, are still being invalidated in contrast to
/// boost::container::stable_vector. As an additional optimization,
/// DeallocateElements can be set to false to defer the deallocation of the
/// individual elements to the destruction of the allocator. Only recommended
/// when using an allocator other than the new_delete_resource.
///
/// NOTE: This container propagates the allocator on copy/move assignment and
/// swap, although polymorphic allocators in general do not do so
///
/// NOTE: In theory, we could also use a deque, but it pays at least 80 bytes of
/// struct-size + additional bookkeeping overhead.
///
/// NOTE: We can very likely further optimize this structure, but for now it is
/// sufficient as is.
template <typename T, bool DeallocateElements /*= true*/>
class StableVector final : public StableVectorBase<T> {
public:
  StableVector(std::pmr::polymorphic_allocator<T> Alloc =
                   std::pmr::get_default_resource()) noexcept
      : StableVectorBase<T>(std::move(Alloc)) {}

  StableVector(StableVector &&Other) noexcept = default;

  explicit StableVector(const StableVector &Other)
      : StableVectorBase<T>(Other.Data, Other.Alloc) {
    if constexpr (DeallocateElements) {
      for (auto *&Elem : this->Data) {
        Elem = new (this->Alloc.allocate(1)) T(*Elem);
      }
    } else {
      auto Ptr = this->Alloc.allocate(this->Data.size());
      for (auto *&Elem : this->Data) {
        Elem = new (Ptr) T(*Elem);
        ++Ptr;
      }
    }
  }

  /// Pass by value to enforce the explicit constructor to trigger at the
  /// call-site
  StableVector &operator=(StableVector Other) noexcept {
    swap(*this, Other);
    return *this;
  }
  StableVector &operator=(StableVector &&Other) noexcept {
    swap(*this, Other);
    return *this;
  }

  friend void swap(StableVector &LHS, StableVector &RHS) noexcept {
    std::swap(LHS.Data, RHS.Data);
    auto LHSMRes = LHS.Alloc.resource();
    auto RHSMRes = RHS.Alloc.resource();

    /// The polymorphic_allocator is just a thin wrapper over a pointer to
    /// std::pmr::memory_resource, so why not swapping them???

    static_assert(
        std::is_trivially_destructible_v<std::pmr::polymorphic_allocator<T>>);
    static_assert(
        std::is_nothrow_constructible_v<std::pmr::polymorphic_allocator<T>,
                                        std::pmr::memory_resource *>);

    new (&LHS.Alloc) std::pmr::polymorphic_allocator<T>(RHSMRes);
    new (&RHS.Alloc) std::pmr::polymorphic_allocator<T>(LHSMRes);
  }

  ~StableVector() { clear(); }

  void pop_back() noexcept {
    auto Elem = this->Data.pop_back_val();
    std::destroy_at(Elem);
    if constexpr (DeallocateElements) {
      this->Alloc.deallocate(Elem, 1);
    }
  }

  void pop_back_n(size_t N) noexcept {
    if constexpr (DeallocateElements || !std::is_trivially_destructible_v<T>) {
      for (auto *Elem :
           llvm::reverse(llvm::ArrayRef<T *>(this->Data).take_back(N))) {
        std::destroy_at(Elem);
        if constexpr (DeallocateElements) {
          this->Alloc.deallocate(Elem, 1);
        }
      }
    }

    this->Data.pop_back_n(N);
  }

  void clear() noexcept {
    if constexpr (DeallocateElements || !std::is_trivially_destructible_v<T>) {
      for (auto *Elem : llvm::reverse(this->Data)) {
        std::destroy_at(Elem);
        if constexpr (DeallocateElements) {
          this->Alloc.deallocate(Elem, 1);
        }
      }
    }

    this->Data.clear();
  }

  bool operator==(const StableVector &Other) const
      noexcept(noexcept(std::declval<T>() != std::declval<T>())) {
    if (this->size() != Other.size()) {
      return false;
    }

    for (auto It1 = this->cbegin(), It2 = Other.cbegin(), End = this->cend();
         It1 != End; ++It1, ++It2) {
      if (*It1 != *It2) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const StableVector &Other) const
      noexcept(noexcept(std::declval<T>() != std::declval<T>())) {
    return !(*this == Other);
  }
};

// NOLINTEND(readability-identifier-naming)
} // namespace psr

#endif // PHASAR_UTILS_STABLEVECTOR_H_
