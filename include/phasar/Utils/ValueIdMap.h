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
#include <memory>
#include <new>
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
/// Entries are stored in flat aligned storage; only explicitly inserted entries
/// are ever constructed — there is no upfront default-construction of the
/// entire value array.
///
/// Reallocation moves all live entries via their move constructors and then
/// destroys the originals in one pass. **Move constructors are assumed to be
/// nothrow**.
///
/// The copy constructor provides the basic exception guarantee: if copying any
/// value throws, already-copied values are destroyed and the allocation is
/// freed.
///
/// Iterator and reference validity follows the same rules as std::vector:
/// insertions that exceed the current capacity invalidate all iterators and
/// references.
///
/// \remarks Partially implemented by Claude Sonnet 4.6
///
/// \tparam IdT    Key type. Must satisfy SmallIdType (fits in uint32_t,
///                losslessly convertible to/from size_t).
/// \tparam ValueT Mapped value type.
// operator= takes Other by value -- handles both copy, and move cases
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
template <SmallIdType IdT, typename ValueT> class ValueIdMap {
public:
  // ── Types ──────────────────────────────────────────────────────────────────

  using key_type = IdT;
  using mapped_type = ValueT;
  using value_type = std::pair<const IdT, ValueT>;
  using size_type = size_t;

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
      return {*CurrentKey, *Map->slotPtr(*CurrentKey)};
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

  constexpr ValueIdMap() noexcept = default;

  /// Pre-allocates storage for \p InitialCapacity entries without constructing
  /// any values.
  explicit ValueIdMap(size_t InitialCapacity)
      : Capacity(InitialCapacity),
        Slots(InitialCapacity ? new Slot[InitialCapacity] : nullptr) {
    IsSet.reserve(InitialCapacity);
  }

  ValueIdMap(const ValueIdMap &Other)
      : IsSet(Other.IsSet), Capacity(Other.Capacity) {
    if (!Capacity) {
      return;
    }
    std::unique_ptr<Slot[]> NewSlots(new Slot[Capacity]);
    Other.IsSet.foreach ([&](IdT Key) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      ::new (NewSlots[size_t(Key)].Data) ValueT(*Other.slotPtr(Key));
    });
    Slots = NewSlots.release();
  }

  ValueIdMap(ValueIdMap &&Other) noexcept
      : IsSet(std::move(Other.IsSet)),
        Capacity(std::exchange(Other.Capacity, 0)),
        Slots(std::exchange(Other.Slots, nullptr)) {}

  ValueIdMap &operator=(ValueIdMap Other) {
    swap(Other);
    return *this;
  }

  ~ValueIdMap() {
    clear();
    delete[] Slots;
  }

  constexpr void swap(ValueIdMap &Other) noexcept {
    std::swap(Slots, Other.Slots);
    std::swap(Capacity, Other.Capacity);
    std::swap(IsSet, Other.IsSet);
  }
  friend constexpr void swap(ValueIdMap &Lhs, ValueIdMap &Rhs) noexcept {
    Lhs.swap(Rhs);
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
  [[nodiscard]] ValueT &at(IdT Key) {
    if (!contains(Key)) {
      throw std::out_of_range("ValueIdMap::at: key not found");
    }
    return *slotPtr(Key);
  }
  [[nodiscard]] const ValueT &at(IdT Key) const {
    if (!contains(Key)) {
      throw std::out_of_range("ValueIdMap::at: key not found");
    }
    return *slotPtr(Key);
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

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ::new (Slots[size_t(Key)].Data) ValueT(PSR_FWD(Args)...);
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
      *slotPtr(Key) = std::forward<M>(Obj);
      return {iterator(this, Key), false};
    }
    return try_emplace(Key, std::forward<M>(Obj));
  }

  /// Removes the entry for \p Key. Returns true if an entry was present.
  bool erase(IdT Key) noexcept {
    if (!IsSet.tryErase(Key)) {
      return false;
    }
    slotPtr(Key)->~ValueT();
    return true;
  }

  /// Removes the entry pointed to by \p It and returns an iterator to the
  /// next entry.
  iterator erase(iterator It) noexcept {
    IdT Key = It->first;
    ++It;
    IsSet.erase(Key);
    slotPtr(Key)->~ValueT();
    return It;
  }

  /// Destroys all present entries without releasing the allocated storage.
  void clear() noexcept {
    if constexpr (!std::is_trivially_destructible_v<ValueT>) {
      IsSet.foreach ([&](IdT Key) { slotPtr(Key)->~ValueT(); });
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
  // ── Private types ──────────────────────────────────────────────────────────

  // Holds raw bytes of the right size and alignment for one ValueT.
  // No ValueT object lives here until explicitly placement-new'd.
  struct alignas(ValueT) Slot {
    std::byte Data[sizeof(ValueT)];
  };

  // ── Private helpers ────────────────────────────────────────────────────────

  [[nodiscard]] ValueT *slotPtr(IdT Key) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return std::launder(reinterpret_cast<ValueT *>(Slots[size_t(Key)].Data));
  }
  [[nodiscard]] const ValueT *slotPtr(IdT Key) const noexcept {
    return std::launder(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        reinterpret_cast<const ValueT *>(Slots[size_t(Key)].Data));
  }

  [[nodiscard]] constexpr bool inbounds(IdT Key) const noexcept {
    return size_t(Key) < Capacity;
  }

  void grow(size_t NewCap) {
    assert(NewCap > Capacity);
    std::unique_ptr<Slot[]> New(new Slot[NewCap]);

    if constexpr (IsTriviallyRelocatable<ValueT>) {
      // Trivially relocatable: a flat memcpy of all used slots suffices.
      // Uninitialized slots contain indeterminate bytes, which is safe to copy.
      // No per-slot construction or destruction is needed.
      if (Slots) {
        memcpy(New.get(), Slots, Capacity * sizeof(Slot));
      }
    } else {
      // Move-construct each live entry into new storage and destroy the
      // original in one pass. Assumes nothrow move construction; types with
      // throwing moves are not supported.
      IsSet.foreach ([&](IdT Key) {
        ValueT *Src = slotPtr(Key);
        ::new (New[size_t(Key)].Data) ValueT(std::move(
            *Src)); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        Src->~ValueT();
      });
    }

    delete[] Slots;
    Slots = New.release();
    Capacity = NewCap;
  }

  // ── Data members ───────────────────────────────────────────────────────────

  // IsSet must be declared first so that if its copy constructor throws during
  // ValueIdMap's copy constructor, Slots has not yet been allocated.
  BitSet<IdT> IsSet;
  size_t Capacity = 0;
  Slot *Slots = nullptr;
};

} // namespace psr

#endif // PHASAR_UTILS_VALUEIDMAP_H