/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_VALUEIDMAP_H
#define PHASAR_UTILS_VALUEIDMAP_H

#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/TypeTraits.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <memory_resource>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace psr {

/// \brief A dense, partially-populated map keyed by a sequential integer-like
/// id.
///
/// Models a subset of the std::unordered_map / llvm::DenseMap interface for
/// keys that are dense unsigned integer ids (e.g. enum classes over uint32_t).
/// Entries are stored in a flat array of uninitialized ValueT storage;
/// only explicitly inserted entries are ever constructed — there is no upfront
/// default-construction of the entire value array.
///
/// Reallocation moves all live entries via their move constructors and then
/// destroys the originals in one pass. **Move constructors are assumed to be
/// nothrow**.
///
/// \remarks Partially implemented by Claude Sonnet 4.6
///
/// \tparam IdT       Key type. Must satisfy SmallIdType (fits in uint32_t,
///                   losslessly convertible to/from size_t).
/// \tparam ValueT    Mapped value type.
/// \tparam Allocator Allocator whose value_type is ValueT.
///                   Defaults to std::allocator<ValueT>.
template <SmallIdType IdT, typename ValueT,
          typename Allocator = std::allocator<ValueT>>
class ValueIdMap {
  using AllocTraits = std::allocator_traits<Allocator>;

public:
  // ── Types ──────────────────────────────────────────────────────────────────

  using key_type = IdT;
  using mapped_type = ValueT;
  using value_type = std::pair<const IdT, ValueT>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using allocator_type = Allocator;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;

  // ── Iterators ──────────────────────────────────────────────────────────────

  /// Forward iterator over present entries. Dereferences to
  /// std::pair<const IdT, ValueT&> by value; prefer structured bindings
  /// (`auto [k, v]`) over `auto&` when iterating.
  template <bool IsConst> class IteratorImpl {
  public:
    using MappedRef = std::conditional_t<IsConst, const ValueT &, ValueT &>;
    using reference = std::pair<const IdT, MappedRef>;
    using value_type = std::pair<const IdT, ValueT>;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr IteratorImpl() noexcept = default;

    /// Implicit conversion iterator → const_iterator.
    template <bool WasConst>
      requires(IsConst && !WasConst)
    constexpr IteratorImpl(const IteratorImpl<WasConst> &Other) noexcept
        : Map(Other.Map), CurrentKey(Other.CurrentKey) {}

    constexpr reference operator*() const noexcept {
      return {*CurrentKey, *Map->slot(*CurrentKey)};
    }

    /// Arrow proxy enabling `it->first` and `it->second`.
    struct ArrowProxy {
      reference Ref;
      constexpr reference *operator->() noexcept { return &Ref; }
    };
    constexpr ArrowProxy operator->() const noexcept { return {**this}; }

    constexpr IteratorImpl &operator++() noexcept {
      CurrentKey = Map->IsSet.findNext(*CurrentKey);
      return *this;
    }
    constexpr IteratorImpl operator++(int) noexcept {
      auto Ret = *this;
      ++*this;
      return Ret;
    }

    constexpr bool
    operator==(const IteratorImpl &Other) const noexcept = default;

  private:
    using MapPtr =
        std::conditional_t<IsConst, const ValueIdMap *, ValueIdMap *>;

    template <bool> friend class IteratorImpl;
    friend class ValueIdMap;

    constexpr IteratorImpl(MapPtr Map, std::optional<IdT> Key) noexcept
        : Map(Map), CurrentKey(Key) {}

    MapPtr Map = nullptr;
    std::optional<IdT> CurrentKey{};
  };

  using iterator = IteratorImpl<false>;
  using const_iterator = IteratorImpl<true>;

  // ── Constructors / destructor ──────────────────────────────────────────────

  /// Default constructor. The allocator is value-initialised.
  ValueIdMap() noexcept(std::is_nothrow_default_constructible_v<Allocator>) =
      default;

  /// Constructs an empty map with the given allocator.
  constexpr explicit ValueIdMap(const allocator_type &A) noexcept(
      std::is_nothrow_copy_constructible_v<Allocator>)
      : Alloc(A) {}

  /// Pre-allocates storage for \p InitialCapacity entries without constructing
  /// any values.
  explicit ValueIdMap(size_t InitialCapacity,
                      const allocator_type &A = allocator_type{})
      : Alloc(A), Capacity(InitialCapacity),
        Slots(InitialCapacity ? AllocTraits::allocate(Alloc, InitialCapacity)
                              : nullptr) {
    IsSet.reserve(InitialCapacity);
  }

  /// Constructs from an initializer list. \p V need not equal \p ValueT;
  /// any type constructible to \p ValueT is accepted, avoiding an
  /// intermediate copy (e.g. string literals when ValueT is std::string).
  /// Duplicate keys are silently dropped (first occurrence wins).
  template <typename V = ValueT>
    requires std::is_constructible_v<ValueT, const V &>
  ValueIdMap(std::initializer_list<std::pair<const IdT, V>> Init,
             const allocator_type &A = allocator_type{})
      : ValueIdMap(Init.begin(), Init.end(), A) {}

  /// Constructs from an iterator range \p [First, Last).
  /// Duplicate keys are silently dropped (first occurrence wins).
  template <typename InputIt>
  ValueIdMap(InputIt First, InputIt Last,
             const allocator_type &A = allocator_type{})
      : ValueIdMap(A) {
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag,
                                    typename std::iterator_traits<
                                        InputIt>::iterator_category>) {
      reserve(static_cast<size_t>(std::distance(First, Last)));
    }
    for (; First != Last; ++First) {
      try_emplace(First->first, First->second);
    }
  }

  /// Copy constructor. The allocator is obtained via
  /// allocator_traits::select_on_container_copy_construction.
  ValueIdMap(const ValueIdMap &Other)
      : ValueIdMap(Other, AllocTraits::select_on_container_copy_construction(
                              Other.Alloc)) {}

  /// Copy constructor with explicit allocator.
  ValueIdMap(const ValueIdMap &Other, const allocator_type &A)
      : Alloc(A), IsSet(Other.IsSet), Capacity(Other.Capacity) {
    if (!Capacity) {
      return;
    }
    Slots = AllocTraits::allocate(Alloc, Capacity);
    try {
      Other.IsSet.foreach ([&](IdT Key) {
        AllocTraits::construct(Alloc, slot(Key), *Other.slot(Key));
      });
    } catch (...) {
      freeStorage();
      IsSet.clear();
      throw;
    }
  }

  /// Move constructor. The allocator is move-constructed from \p Other.
  ValueIdMap(ValueIdMap &&Other) noexcept
      : Alloc(std::move(Other.Alloc)), IsSet(std::move(Other.IsSet)),
        Capacity(std::exchange(Other.Capacity, 0)),
        Slots(std::exchange(Other.Slots, nullptr)) {}

  /// Move constructor with explicit allocator.
  ///
  /// If the supplied allocator compares equal to Other's allocator the
  /// storage is stolen in O(1); otherwise each live element is move-constructed
  /// individually using the supplied allocator.
  ValueIdMap(ValueIdMap &&Other, const allocator_type &A) : Alloc(A) {
    if (Alloc == Other.Alloc) {
      stealFrom(Other);
    } else {
      // Allocators differ: move elements individually into fresh storage.
      IsSet = Other.IsSet;
      if (Other.Capacity) {
        Capacity = Other.Capacity;
        Slots = AllocTraits::allocate(Alloc, Capacity);
        Other.IsSet.foreach ([&](IdT Key) {
          ValueT *Src = Other.slot(Key);
          AllocTraits::construct(Alloc, slot(Key), std::move(*Src));
          AllocTraits::destroy(Other.Alloc, Src);
        });
        Other.IsSet.clear();
        Other.freeStorage();
      }
    }
  }

  /// Copy assignment. Propagates the allocator when
  /// allocator_traits::propagate_on_container_copy_assignment is true.
  ValueIdMap &operator=(const ValueIdMap &Other) {
    if (this == &Other) {
      return *this;
    }
    clear();
    if constexpr (AllocTraits::propagate_on_container_copy_assignment::value) {
      if (Alloc != Other.Alloc) {
        freeStorage();
        Alloc = Other.Alloc;
      }
    }
    reallocAtLeast(Other.Capacity);
    IsSet = Other.IsSet;
    Other.IsSet.foreach ([&](IdT Key) {
      AllocTraits::construct(Alloc, slot(Key), *Other.slot(Key));
    });
    return *this;
  }

  /// Replaces the contents with those of \p Init.
  /// Duplicate keys are silently dropped (first occurrence wins).
  template <typename V = ValueT>
    requires std::is_constructible_v<ValueT, const V &>
  ValueIdMap &operator=(std::initializer_list<std::pair<const IdT, V>> Init) {
    clear();
    reserve(Init.size());
    for (auto &&[Key, Val] : Init) {
      try_emplace(Key, Val);
    }
    return *this;
  }

  /// Move assignment. Propagates the allocator when
  /// allocator_traits::propagate_on_container_move_assignment is true.
  ValueIdMap &operator=(ValueIdMap &&Other) noexcept(
      AllocTraits::propagate_on_container_move_assignment::value ||
      AllocTraits::is_always_equal::value) {
    if (this == &Other) {
      return *this;
    }
    clear();
    if constexpr (AllocTraits::propagate_on_container_move_assignment::value) {
      freeStorage();
      Alloc = std::move(Other.Alloc);
      stealFrom(Other);
    } else if (Alloc == Other.Alloc) {
      freeStorage();
      stealFrom(Other);
    } else {
      // Unequal allocators, no propagation: move elements individually.
      reallocAtLeast(Other.Capacity);
      IsSet = Other.IsSet;
      Other.IsSet.foreach ([&](IdT Key) {
        ValueT *Src = Other.slot(Key);
        AllocTraits::construct(Alloc, slot(Key), std::move(*Src));
        AllocTraits::destroy(Other.Alloc, Src);
      });
      Other.IsSet.clear();
      Other.freeStorage();
    }
    return *this;
  }

  ~ValueIdMap() {
    clear();
    freeStorage();
  }

  /// Returns the allocator associated with this container.
  // NOLINTNEXTLINE(readability-identifier-naming) -- STL API name
  [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
    return Alloc;
  }

  /// Swap. Propagates allocators when
  /// allocator_traits::propagate_on_container_swap is true; otherwise the
  /// behaviour is undefined if the two allocators are not equal (per the
  /// AllocatorAwareContainer requirement).
  void swap(ValueIdMap &Other) noexcept(
      (!AllocTraits::propagate_on_container_swap::value ||
       std::is_nothrow_swappable_v<Allocator>) &&
      std::is_nothrow_swappable_v<BitSet<IdT>>) {
    if constexpr (AllocTraits::propagate_on_container_swap::value) {
      std::swap(Alloc, Other.Alloc);
    }
    std::swap(Slots, Other.Slots);
    std::swap(Capacity, Other.Capacity);
    std::swap(IsSet, Other.IsSet);
  }
  friend void swap(ValueIdMap &Lhs,
                   ValueIdMap &Rhs) noexcept(noexcept(Lhs.swap(Rhs))) {
    Lhs.swap(Rhs);
  }

  friend bool operator==(const ValueIdMap &Lhs, const ValueIdMap &Rhs) noexcept(
      noexcept(std::declval<const ValueT &>() ==
               std::declval<const ValueT &>())) {
    if (Lhs.IsSet != Rhs.IsSet) {
      return false;
    }
    for (auto [Key, Val] : Lhs) {
      if (*Rhs.slot(Key) != Val) {
        return false;
      }
    }
    return true;
  }

  // ── Capacity ───────────────────────────────────────────────────────────────

  /// Ensures storage for at least \p N entries without constructing any values.
  void reserve(size_t N) {
    if (N > Capacity) {
      IsSet.reserve(N);
      grow(N);
    }
  }

  /// Returns the number of entries currently present.
  [[nodiscard]] constexpr size_t size() const noexcept { return IsSet.size(); }
  [[nodiscard]] constexpr bool empty() const noexcept { return IsSet.empty(); }
  /// Returns the maximum number of elements the container can hold.
  /// Capped at INT_MAX because llvm::BitVector (which backs BitSet) uses
  /// an int-sized bit count internally.
  // NOLINTNEXTLINE(readability-identifier-naming) -- STL API name
  [[nodiscard]] constexpr size_type max_size() const noexcept {
    return std::min(AllocTraits::max_size(Alloc),
                    size_type(std::numeric_limits<int>::max()));
  }

  // ── Lookup ─────────────────────────────────────────────────────────────────

  [[nodiscard]] constexpr bool contains(IdT Key) const noexcept {
    return IsSet.contains(Key);
  }
  [[nodiscard]] constexpr size_t count(IdT Key) const noexcept {
    return contains(Key) ? 1 : 0;
  }

  [[nodiscard]] constexpr iterator find(IdT Key) noexcept {
    return contains(Key) ? iterator(this, Key) : end();
  }
  [[nodiscard]] constexpr const_iterator find(IdT Key) const noexcept {
    return contains(Key) ? const_iterator(this, Key) : end();
  }

  /// Returns a reference to the value for \p Key.
  /// \throws std::out_of_range if \p Key is not present.
  [[nodiscard]] constexpr ValueT &at(IdT Key) {
    if (!contains(Key)) {
      throw std::out_of_range("ValueIdMap::at: key not found");
    }
    return *slot(Key);
  }
  [[nodiscard]] constexpr const ValueT &at(IdT Key) const {
    if (!contains(Key)) {
      throw std::out_of_range("ValueIdMap::at: key not found");
    }
    return *slot(Key);
  }

  // ── Element access ─────────────────────────────────────────────────────────

  /// Returns a reference to the value for \p Key, default-constructing it if
  /// absent (same semantics as std::unordered_map::operator[]).
  ValueT &operator[](IdT Key) { return try_emplace(Key).first->second; }

  // ── Modifiers ─────────────────────────────────────────────────────────────

  /// Constructs a value from \p Args for \p Key if not already present.
  /// Returns {iterator-to-entry, true} on insertion and
  /// {iterator-to-existing-entry, false} otherwise.
  template <typename... ArgsT>
  std::pair<iterator, bool> try_emplace(IdT Key, ArgsT &&...Args) {
    if (!inbounds(Key)) {
      grow(std::max(size_t(Key) + 1, Capacity ? Capacity * 2 : 8));
    } else if (IsSet.contains(Key)) {
      return {iterator(this, Key), false};
    }
    AllocTraits::construct(Alloc, slot(Key), PSR_FWD(Args)...);
    IsSet.insert(Key);
    return {iterator(this, Key), true};
  }

  std::pair<iterator, bool> insert(value_type Val) {
    return try_emplace(Val.first, std::move(Val.second));
  }

  /// Inserts or replaces the entry for \p Key with \p Obj.
  template <typename M>
  // NOLINTNEXTLINE(readability-identifier-naming) -- STL API name
  std::pair<iterator, bool> insert_or_assign(IdT Key, M &&Obj) {
    if (IsSet.contains(Key)) {
      *slot(Key) = std::forward<M>(Obj);
      return {iterator(this, Key), false};
    }
    return try_emplace(Key, std::forward<M>(Obj));
  }

  /// Removes the entry for \p Key. Returns true if an entry was present.
  bool erase(IdT Key) noexcept {
    if (!IsSet.tryErase(Key)) {
      return false;
    }
    AllocTraits::destroy(Alloc, slot(Key));
    return true;
  }

  /// Removes the entry pointed to by \p It and returns an iterator to the
  /// next entry.
  iterator erase(const_iterator It) noexcept {
    IdT Key = It->first;
    ++It;
    IsSet.erase(Key);
    AllocTraits::destroy(Alloc, slot(Key));
    return iterator(this, It.CurrentKey);
  }

  /// Destroys all present entries without releasing the allocated storage.
  void clear() noexcept {
    if constexpr (!std::is_trivially_destructible_v<ValueT>) {
      IsSet.foreach ([&](IdT Key) { AllocTraits::destroy(Alloc, slot(Key)); });
    }
    IsSet.clear();
  }

  // ── Iteration ─────────────────────────────────────────────────────────────

  [[nodiscard]] constexpr iterator begin() noexcept {
    return {this, IsSet.findFirst()};
  }
  [[nodiscard]] constexpr iterator end() noexcept {
    return {this, std::nullopt};
  }
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return {this, IsSet.findFirst()};
  }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return {this, std::nullopt};
  }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return begin();
  }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

private:
  // ── Private helpers ────────────────────────────────────────────────────────

  [[nodiscard]] constexpr ValueT *slot(IdT Key) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return Slots + size_t(Key);
  }
  [[nodiscard]] constexpr const ValueT *slot(IdT Key) const noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return Slots + size_t(Key);
  }

  [[nodiscard]] constexpr bool inbounds(IdT Key) const noexcept {
    return size_t(Key) < Capacity;
  }

  /// Frees current storage and allocates a fresh array of \p N elements.
  /// No-op if N <= Capacity.
  constexpr void reallocAtLeast(size_t N) {
    if (N > Capacity) {
      freeStorage();
      Slots = AllocTraits::allocate(Alloc, N);
      Capacity = N;
    }
  }

  /// Deallocates the storage array if non-null and resets Slots/Capacity.
  constexpr void freeStorage() noexcept {
    if (Slots) {
      AllocTraits::deallocate(Alloc, Slots, Capacity);
      Slots = nullptr;
      Capacity = 0;
    }
  }

  /// Takes ownership of Other's storage (Other must have been cleared first).
  void stealFrom(ValueIdMap &Other) noexcept {
    IsSet = std::move(Other.IsSet);
    Capacity = std::exchange(Other.Capacity, 0);
    Slots = std::exchange(Other.Slots, nullptr);
  }

  void grow(size_t NewCap) {
    assert(NewCap > Capacity);
    ValueT *New = AllocTraits::allocate(Alloc, NewCap);

    if constexpr (IsTriviallyRelocatable<ValueT>) {
      // Trivially relocatable: a flat memcpy of all used slots suffices.
      // Uninitialized slots contain indeterminate bytes, which is safe to copy.
      // No per-element construction or destruction is needed.
      if (Slots) {
        memcpy(New, Slots, Capacity * sizeof(ValueT));
      }
    } else {
      // Move-construct each live entry into new storage and destroy the
      // original in one pass. Assumes nothrow move construction; types with
      // throwing moves are not supported.
      IsSet.foreach ([&](IdT Key) {
        ValueT *Src = slot(Key);
        AllocTraits::construct(Alloc, New + size_t(Key), // NOLINT
                               std::move(*Src));
        AllocTraits::destroy(Alloc, Src);
      });
    }

    if (Slots) {
      AllocTraits::deallocate(Alloc, Slots, Capacity);
    }
    Slots = New;
    Capacity = NewCap;
  }

  // ── Data members ───────────────────────────────────────────────────────────

  // Alloc must be declared first so it is available for deallocation when
  // other members are destroyed. [[no_unique_address]] enables the empty-base
  // optimisation for stateless allocators such as std::allocator.
  [[no_unique_address]] Allocator Alloc{};
  // IsSet must be declared before Slots so that if its copy constructor throws
  // during ValueIdMap's copy constructor, Slots has not yet been allocated.
  BitSet<IdT> IsSet;
  size_t Capacity = 0;
  ValueT *Slots = nullptr;
};

namespace pmr {
template <SmallIdType IdT, typename ValueT>
using ValueIdMap =
    psr::ValueIdMap<IdT, ValueT, std::pmr::polymorphic_allocator<ValueT>>;
} // namespace pmr

} // namespace psr

#endif // PHASAR_UTILS_VALUEIDMAP_H
