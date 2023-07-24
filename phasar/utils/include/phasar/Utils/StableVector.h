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

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>

namespace psr {

// NOLINTBEGIN(readability-identifier-naming)

/// A special-purpose container designed for providing the almost complete
/// interface from std::vector, but with the guarantee that references to stored
/// elements are not invalidated by insertion or deletion of other elements.
///
/// This container can be used as memory owner for objects of any size
/// providing the allocator properties of std::pmr::monotonoc_buffer_resource
/// but with the convenience of a standard container.
///
/// NOTE: This container performs slightly to very much better than std::deque
/// depending on the task.
template <typename T, typename Allocator = std::allocator<T>>
class StableVector {

  constexpr static size_t InitialLogCapacity = 5;
  constexpr static size_t InitialCapacity = size_t(1) << InitialLogCapacity;

public:
  template <bool IsConst> class Iterator {
  public:
    using value_type = std::conditional_t<IsConst, const T, T>;
    using reference = value_type &;
    using pointer = value_type *;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    Iterator &operator++() noexcept {
      if (LLVM_LIKELY(++It != ItEnd)) {
/// Let the compiler optimize away the operator== call entirely
#if __has_builtin(__builtin_assume)
        __builtin_assume(It != nullptr);
#endif
        return *this;
      }

      if (Outer == OuterEnd - 1) {
        // We are at the end of the tail loop
        It = nullptr;
        return *this;
      }
      if (++Outer == OuterEnd - 1) {
        // We are at the end of the main loop => enter the tail loop now
        It = *Outer;
        ItEnd = Pos;
#if __has_builtin(__builtin_assume)
        __builtin_assume(It != nullptr);
#endif
        return *this;
      }
      // We are still in the main loop

      It = *Outer;
      ItEnd = It + Total;

      Total <<= 1;
#if __has_builtin(__builtin_assume)
      __builtin_assume(It != nullptr);
#endif
      return *this;
    }

    Iterator operator++(int) noexcept {
      auto Ret = *this;
      ++*this;
      return Ret;
    }

    [[nodiscard]] reference operator*() const noexcept { return *It; }
    [[nodiscard]] pointer operator->() const noexcept { return It; }

    [[nodiscard]] bool operator==(const Iterator &Other) const noexcept {
      return It == Other.It;
    }

    [[nodiscard]] bool operator!=(const Iterator &Other) const noexcept {
      return !(*this == Other);
    }

    Iterator(const Iterator &) noexcept = default;
    template <bool C, typename = std::enable_if_t<!C && IsConst>>
    Iterator(const Iterator<C> &Other) noexcept
        : It(Other.It), ItEnd(Other.ItEnd), Outer(Other.Outer),
          OuterEnd(Other.OuterEnd), Total(Other.Total), Pos(Other.Pos) {}

    ~Iterator() = default;

    Iterator &operator=(const Iterator &) noexcept = default;
    template <bool C, typename = std::enable_if_t<!C && IsConst>>
    Iterator &operator=(const Iterator<C> &Other) noexcept {
      new (this) Iterator(Other);
      return *this;
    }

  private:
    friend class StableVector;

    Iterator(T *const *Outer, T *const *OuterEnd, T *Pos) noexcept
        : Outer(Outer), OuterEnd(OuterEnd), Pos(Pos) {
      if (OuterEnd == Outer) {
        return;
      }

      It = *Outer;
      if (OuterEnd - 1 == Outer) {
        ItEnd = Pos;
      } else {
        ItEnd = It + InitialCapacity;
      }
    }

    T *It = nullptr;
    T *ItEnd = nullptr;
    T *const *Outer = nullptr;
    T *const *OuterEnd = nullptr;
    size_t Total = InitialCapacity;
    T *Pos = nullptr;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using value_type = T;
  using reference = value_type &;
  using const_reference = const value_type &;
  using difference_type = ptrdiff_t;
  using size_type = size_t;
  using allocator_type =
      typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

  StableVector(const allocator_type &Alloc = allocator_type()) noexcept(
      std::is_nothrow_copy_constructible_v<allocator_type>)
      : Alloc(Alloc) {}

  StableVector(StableVector &&Other) noexcept
      : Blocks(std::move(Other.Blocks)), Start(Other.Start), Pos(Other.Pos),
        End(Other.End), Size(Other.Size), BlockIdx(Other.BlockIdx),
        Alloc(std::move(Other.Alloc)) {
    Other.Start = nullptr;
    Other.Pos = nullptr;
    Other.End = nullptr;
    Other.Size = 0;
    Other.BlockIdx = 0;
  }

  explicit StableVector(const StableVector &Other)
      : Size(Other.Size), BlockIdx(Other.BlockIdx),
        Alloc(std::allocator_traits<allocator_type>::
                  select_on_container_copy_construction(Other.Alloc)) {
    if (Other.empty()) {
      return;
    }
    Blocks.reserve(BlockIdx + 1);
    auto Cap = InitialCapacity;
    auto Total = InitialCapacity;

    for (size_t I = 0; I < BlockIdx; ++I) {
      auto Blck =
          (T *)std::allocator_traits<allocator_type>::allocate(Alloc, Cap);

      std::uninitialized_copy_n(Other.Blocks[I], Cap, Blck);
      Blocks.push_back(Blck);

      Cap = Total;
      Total <<= 1;
    }

    auto Blck =
        (T *)std::allocator_traits<allocator_type>::allocate(Alloc, Cap);
    std::uninitialized_copy(Other.Start, Other.Pos, Blck);
    Blocks.push_back(Blck);

    Start = Blck;
    End = Blck + Cap;
    Pos = Blck + (Other.Pos - Other.Start);
  }

  void swap(StableVector &Other) noexcept {
    std::swap(Blocks, Other.Blocks);
    std::swap(Start, Other.Start);
    std::swap(Pos, Other.Pos);
    std::swap(End, Other.End);
    std::swap(Size, Other.Size);
    std::swap(BlockIdx, Other.BlockIdx);

    if constexpr (std::allocator_traits<
                      allocator_type>::propagate_on_container_swap::value) {
      std::swap(Alloc, Other.Alloc);
    } else {
      assert(Alloc == Other.Alloc &&
             "Do not swap two StableVectors with incompatible "
             "allocators that do not propagate on swap!");
    }
  }
  friend void swap(StableVector &LHS, StableVector &RHS) noexcept {
    LHS.swap(RHS);
  }

  // This would be silently expensive... If you really want this, call clone()
  StableVector &operator=(const StableVector &) = delete;

  StableVector &operator=(StableVector &&Other) noexcept {
    swap(Other);
    return *this;
  }

  ~StableVector() {
    auto Cap = InitialCapacity;
    auto TotalSize = Cap;

    for (size_t I = 0; I < BlockIdx; ++I) {
      std::destroy_n(Blocks[I], Cap);

      std::allocator_traits<allocator_type>::deallocate(Alloc, Blocks[I], Cap);

      Cap = TotalSize;
      TotalSize <<= 1;
    }

    std::destroy(Start, Pos);

    for (size_t I = BlockIdx; I < Blocks.size(); ++I) {
      std::allocator_traits<allocator_type>::deallocate(Alloc, Blocks[I], Cap);

      Cap = TotalSize;
      TotalSize <<= 1;
    }

    Blocks.clear();
  }

  /// Due to the 'explicit' copy ctor, creating copies may be cumbersome, i.e.
  /// having to specify the whole type. So, provide a utility function for it
  [[nodiscard]] StableVector clone() const { return StableVector(*this); }

  template <typename... ArgTys> T &emplace_back(ArgTys &&...Args) {
    if (Pos == End) {
      return growAndEmplace(std::forward<ArgTys>(Args)...);
    }

    auto Ret = Pos;
    std::allocator_traits<allocator_type>::construct(
        Alloc, Ret, std::forward<ArgTys>(Args)...);
    ++Pos;
    ++Size;
    return *Ret;
  }

  void push_back(const T &Elem) { emplace_back(Elem); }
  void push_back(T &&Elem) { emplace_back(std::move(Elem)); }

  [[nodiscard]] T &operator[](size_t Index) noexcept {
    return subscriptHelper(Index);
  }

  [[nodiscard]] const T &operator[](size_t Index) const noexcept {
    return subscriptHelper(Index);
  }

  [[nodiscard]] iterator begin() noexcept {
    if (empty()) {
      return end();
    }
    return {Blocks.begin(), Blocks.begin() + BlockIdx + 1, Pos};
  }
  [[nodiscard]] iterator end() noexcept {
    return {Blocks.begin() + BlockIdx + 1, Blocks.begin() + BlockIdx + 1,
            nullptr};
  }

  [[nodiscard]] const_iterator cbegin() const noexcept {
    if (empty()) {
      return cend();
    }
    return {Blocks.begin(), Blocks.begin() + BlockIdx + 1, Pos};
  }
  [[nodiscard]] const_iterator cend() const noexcept {
    return {Blocks.begin() + BlockIdx + 1, Blocks.begin() + BlockIdx + 1,
            nullptr};
  }

  [[nodiscard]] const_iterator begin() const noexcept { return cbegin(); }
  [[nodiscard]] const_iterator end() const noexcept { return cend(); }

  [[nodiscard]] size_t size() const noexcept { return Size; }
  [[nodiscard]] bool empty() const noexcept { return Size == 0; }

  [[nodiscard]] size_t max_size() const noexcept {
    /// In theory, we could allocate as many blocks as necessary, such that the
    /// accumulated size of them is exactly SIZE_MAX+1, but this will already
    /// overflow the Size field and we still need to store the blocks and the
    /// other metadata somewhere, such that we will never be able to allocate
    /// the last block (with size SIZE_MAX/2). So, the maximum number of
    /// elements will be SIZE_MAX/2;
    return (SIZE_MAX / 2) / sizeof(T);
  };

  [[nodiscard]] T &front() noexcept {
    assert(!empty() && "Do not call front() on an empty StableVector!");
    return *Blocks[0];
  }

  [[nodiscard]] const T &front() const noexcept {
    assert(!empty() && "Do not call front() on an empty StableVector!");
    return *Blocks[0];
  }

  [[nodiscard]] T &back() noexcept {
    assert(!empty() && "Do not call back() on an empty StableVector!");
    return Pos[-1];
  }

  [[nodiscard]] const T &back() const noexcept {
    assert(!empty() && "Do not call back() on an empty StableVector!");
    return Pos[-1];
  }

  void pop_back() noexcept {
    assert(!empty() && "Do not call pop_back() on an empty StableVector!");

    std::destroy_at(--Pos);
    --Size;
    if (Pos != Start) {
      return;
    }

    if (BlockIdx) {
      if (--BlockIdx) {
        auto BlockSize = size_t(End - Start);
        assert(llvm::isPowerOf2_64(BlockSize));
        assert(BlockSize == Size);
        Start = Blocks[BlockIdx];
        End = Pos = Start + BlockSize / 2;
      } else {
        Start = Blocks[0];
        End = Pos = Start + InitialCapacity;
      }
    }
  }

  [[nodiscard]] T pop_back_val() noexcept {
    auto Ret = std::move(back());
    pop_back();
    return Ret;
  }

  void clear() noexcept {
    auto Cap = InitialCapacity;
    auto TotalSize = Cap;

    for (size_t I = 0; I < BlockIdx; ++I) {
      std::destroy_n(Blocks[I], Cap);
      Cap = TotalSize;
      TotalSize += Cap;
    }

    std::destroy(Start, Pos);
    BlockIdx = 0;
    Size = 0;
    if (!Blocks.empty()) {
      Start = Pos = Blocks.front();
      End = Start + InitialCapacity;
    }
  }

  void pop_back_n(size_t N) noexcept {
    assert(Size >= N && "Do not call pop_back() on an empty StableVector!");
    if (!N) {
      return;
    }

    do {
      auto NumElementsInCurrBlock = size_t(Pos - Start);
      if (NumElementsInCurrBlock > N) {
        Pos -= N;
        Size -= N;
        std::destroy_n(Pos, N);
        return;
      }

      std::destroy(Start, Pos);
      Size -= NumElementsInCurrBlock;
      N -= NumElementsInCurrBlock;

      if (BlockIdx) {
        if (--BlockIdx) {
          auto BlockSize = size_t(End - Start);
          Start = Blocks[BlockIdx];
          Pos = End = Start + BlockSize / 2;
        } else {
          Start = Blocks.front();
          Pos = End = Start + InitialCapacity;
        }
      } else {
        Start = Pos = Blocks.front();
        End = Start + InitialCapacity;
      }
    } while (N);
  }

  void shrink_to_fit() noexcept {
    if (Blocks.empty() || BlockIdx == Blocks.size() - 1) {
      return;
    }

    if (Size == 0) {
      assert(BlockIdx == 0);
      std::allocator_traits<allocator_type>::deallocate(Alloc, Blocks[0],
                                                        InitialCapacity);
    }

    auto Cap = size_t(1) << (BlockIdx + InitialLogCapacity);

    for (size_t I = BlockIdx + 1, BlocksEnd = Blocks.size(); I < BlocksEnd;
         ++I) {
      std::allocator_traits<allocator_type>::deallocate(Alloc, Blocks[I], Cap);
      Cap <<= 1;
    }

    Blocks.resize(BlockIdx + (Size != 0));
  }

  [[nodiscard]] friend bool operator==(const StableVector &LHS,
                                       const StableVector &RHS) noexcept {
    if (LHS.size() != RHS.size()) {
      return false;
    }
    if (LHS.empty()) {
      return true;
    }

    size_t Cap = InitialCapacity;
    size_t Total = InitialCapacity;

    for (size_t I = 0; I < LHS.BlockIdx; ++I) {
      for (T *LIt = LHS.Blocks[I], *RIt = RHS.Blocks[I], *LEnd = LIt + Cap;
           LIt != LEnd; ++LIt, ++RIt) {
        if (*LIt != *RIt) {
          return false;
        }
      }

      Cap = Total;
      Total <<= 1;
    }

    for (T *LIt = LHS.Start, *RIt = RHS.Start, *LEnd = LHS.Pos; LIt != LEnd;
         ++LIt, ++RIt) {
      if (*LIt != *RIt) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] friend bool operator!=(const StableVector &LHS,
                                       const StableVector &RHS) noexcept {
    return !(LHS == RHS);
  }

  [[nodiscard]] allocator_type get_allocator() const
      noexcept(std::is_nothrow_copy_constructible_v<allocator_type>) {
    return Alloc;
  }

private:
  template <typename... ArgTys>
  [[nodiscard]] T &growAndEmplace(ArgTys &&...Args) {
    auto makeBlock = [this](size_t N) {
      return std::allocator_traits<allocator_type>::allocate(Alloc, N);
    };

    if (Blocks.empty()) {
      Blocks.push_back(makeBlock(InitialCapacity));
      End = Blocks.back() + InitialCapacity;
    } else if (BlockIdx < Blocks.size() - 1) {
      assert(llvm::isPowerOf2_64(Size));
      BlockIdx++;
      End = Blocks[BlockIdx] + Size;
    } else {
      assert(llvm::isPowerOf2_64(Size));
      BlockIdx++;
      Blocks.push_back(makeBlock(Size));
      End = Blocks.back() + Size;
    }

    auto *Ret = Blocks[BlockIdx];

    Start = Ret;
    Pos = Ret + 1;
    ++Size;

    std::allocator_traits<allocator_type>::construct(
        Alloc, Ret, std::forward<ArgTys>(Args)...);
    return *Ret;
  }

  [[nodiscard]] inline T &subscriptHelper(size_t Index) const noexcept {
    if (Index < InitialCapacity) {
      return Blocks[0][Index];
    }

    auto Log = llvm::Log2_64(Index);
    auto LogIdx = Log - (InitialLogCapacity - 1);
    auto Offset = Index - (size_t(1) << Log);
    return Blocks[LogIdx][Offset];

    // auto GreaterEqual32Mask = -(Index >= InitialCapacity);
    // auto Log = llvm::Log2_64_Ceil(Index + 1);
    // // Really wish to call @llvm.usub.sat here...
    // auto LogIdx = (Log - InitialLogCapacity) & GreaterEqual32Mask;
    // auto Offset = Index & (((size_t(1) << Log) >> 1) - 1);
    // return Blocks[LogIdx][Offset];
  }

  llvm::SmallVector<T *, 0> Blocks;
  T *Start = nullptr;
  T *Pos = nullptr;
  T *End = nullptr;
  size_t Size = 0;
  size_t BlockIdx = 0;
  [[no_unique_address]] allocator_type Alloc;
};

// NOLINTEND(readability-identifier-naming)
} // namespace psr

#endif // PHASAR_UTILS_STABLEVECTOR_H_
