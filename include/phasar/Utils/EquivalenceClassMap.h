/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Florian Sattler and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_EQUIVALENCECLASSMAP_H
#define PHASAR_UTILS_EQUIVALENCECLASSMAP_H

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator_range.h"

#include <functional>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <set>

namespace psr {

// EquivalenceClassMap is a special map type that splits the keys into
// equivalence classes regarding their mapped values. Meaning, that all keys
// that are equivalent are mapped to the same value. Two keys are treated as
// equivalent and merged into a equivalence class when they refer to Values
// that compare equal.
template <typename KeyT, typename ValueT> struct EquivalenceClassMap {
  template <typename... Ts> using SetType = std::set<Ts...>;
  using EquivalenceClassBucketT = std::pair<SetType<KeyT>, ValueT>;
  using StorageT = llvm::SmallVector<EquivalenceClassBucketT, 0>;

public:
  using size_type = size_t;
  using key_type = KeyT;
  using mapped_type = ValueT;
  using value_type = EquivalenceClassBucketT;

  using const_iterator = typename StorageT::const_iterator;

  using insert_return_type =
      std::pair<typename SetType<KeyT>::const_iterator, bool>;

  EquivalenceClassMap(unsigned InitialEquivalenceClasses = 0) {
    StoredData.reserve(InitialEquivalenceClasses);
  }

  template <typename InputIt>
  EquivalenceClassMap(const InputIt &I, const InputIt &End) {
    this->insert(I, End);
  }

  EquivalenceClassMap(
      std::initializer_list<std::pair<key_type, mapped_type>> Vals) {
    this->insert(Vals.begin(), Vals.end());
  }

  [[nodiscard]] inline const_iterator begin() const {
    return StoredData.begin();
  }
  [[nodiscard]] inline const_iterator end() const { return StoredData.end(); }
  [[nodiscard]] inline llvm::iterator_range<const_iterator>
  equivalenceClasses() const {
    return llvm::make_range(begin(), end());
  }

  // Inserts Key into the corresponding equivalence class for Value. If Value
  // is not already in the map a new equivalence class is created.
  template <typename ValueType = ValueT>
  insert_return_type insert(const KeyT &Key, ValueType &&Value) {
    return try_emplace(Key, std::forward<ValueType>(Value));
  }

  // Inserts Key into the corresponding equivalence class for Value. If Value
  // is not already in the map a new equivalence class is created.
  template <typename ValueType = ValueT>
  insert_return_type insert(KeyT &&Key, ValueType &&Value) {
    return try_emplace(std::move(Key), std::forward<ValueType>(Value));
  }

  // Inserts Key into the corresponding equivalence class for Value. If Value
  // is not already in the map a new equivalence class is created.
  insert_return_type insert(const std::pair<KeyT, ValueT> &KVPair) {
    return try_emplace(KVPair.first, KVPair.second);
  }

  // Inserts Key into the corresponding equivalence class for Value. If Value
  // is not already in the map a new equivalence class is created.
  insert_return_type insert(std::pair<KeyT, ValueT> &&KVPair) {
    return try_emplace(std::move(KVPair.first), std::move(KVPair.second));
  }

  // Insert a range of Key Values pairs into the map.
  template <typename InputIt> void insert(InputIt I, InputIt End) {
    for (; I != End; ++I) {
      try_emplace(I->first, I->second);
    }
  }

  // Inserts Key into the corresponding equivalence class for Value. If Value
  // is not already in the map a new equivalence class is created.
  template <typename... Ts>
  insert_return_type try_emplace(KeyT &&Key, Ts &&...Args) {
    ValueT Val{std::forward<Ts...>(Args...)};

    for (auto &KVPair : StoredData) {
      if (KVPair.second == Val) {
        return KVPair.first.insert(Key);
      }
    }

    StoredData.emplace_back(SetType<KeyT>{std::move(Key)}, std::move(Val));
    return std::make_pair(StoredData.back().first.begin(), true);
  }

  // Inserts Key into the corresponding equivalence class for Value. If Value
  // is not already in the map a new equivalence class is created.
  template <typename... Ts>
  insert_return_type try_emplace(const KeyT &Key, Ts &&...Args) {
    ValueT Val{std::forward<Ts...>(Args...)};

    for (auto &KVPair : StoredData) {
      if (KVPair.second == Val) {
        return KVPair.first.insert(Key);
      }
    }

    StoredData.emplace_back(SetType<KeyT>{Key}, std::move(Val));
    return std::make_pair(StoredData.back().first.begin(), true);
  }

  // Return 1 if the specified key is in the map, 0 otherwise.
  [[nodiscard]] inline size_type count(const KeyT &Key) const {
    for (auto &KVPair : StoredData) {
      if (KVPair.first.count(Key) >= 1) {
        return 1;
      }
    }
    return 0;
  }

  [[nodiscard]] inline size_type numEquivalenceClasses() const {
    return StoredData.size();
  }

  // Returns the size of the map, i.e., the number of equivalence classes.
  [[nodiscard]] inline size_type size() const {
    return numEquivalenceClasses();
  }

  [[nodiscard]] const_iterator find(key_type Key) const {
    return llvm::find_if(StoredData,
                         [&Key](const EquivalenceClassBucketT &Val) -> bool {
                           return Val.first.count(Key) >= 1;
                         });
  }

  [[nodiscard]] std::optional<ValueT> findValue(key_type Key) const {
    auto Search = find(Key);
    if (Search != StoredData.end()) {
      return Search->second;
    }
    return std::nullopt;
  }

  inline void clear() { StoredData.clear(); }

private:
  StorageT StoredData{};
};

template <typename TKey, typename TValue,
          typename ValueComparator = std::equal_to<TValue>>
class EquivalenceClassMapNG {
  using SetTy = llvm::SmallDenseSet<TKey, 2>;

public:
  // NOLINTNEXTLINE(readability-identifier-naming)
  class const_iterator {

  public:
    using value_type = std::pair<SetTy, TValue>;
    using reference = std::pair<const SetTy &, const TValue &>;
    using pointer = reference *;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    const_iterator &operator++() noexcept {
      ++Val;
      ++Ky;
      return *this;
    }

    reference operator*() noexcept { return reference(*Ky, *Val); }

    pointer operator->() noexcept {
      TempStorage.emplace(*Ky, *Val);
      return &*TempStorage;
    }

    bool operator==(const const_iterator &Other) const noexcept {
      return Val == Other.Val;
    }

    bool operator!=(const const_iterator &Other) const noexcept {
      return !(*this == Other);
    }

  private:
    friend EquivalenceClassMapNG;

    const_iterator(const TValue *Val, const SetTy *Ky) noexcept
        : Val(Val), Ky(Ky) {}

    const TValue *Val;
    const SetTy *Ky;

    std::optional<reference> TempStorage;
  };

  EquivalenceClassMapNG() noexcept = default;

  void reserve(size_t InitialCapacity) {
    Values.reserve(InitialCapacity);
    Keys.reserve(InitialCapacity);
  }

  template <typename KK, typename VV>
  std::pair<const_iterator, bool> insert(KK &&Key, VV &&Value) {
    ValueComparator VComp;
    for (size_t I = 0, End = Values.size(); I != End; ++I) {
      if (VComp(Values[I], Value)) {
        return {getIterator(I), Keys[I].insert(std::forward<KK>(Key)).second};
      }
    }

    Values.emplace_back(std::forward<VV>(Value));
    Keys.emplace_back().insert(std::forward<KK>(Key));
    return {getIterator(Values.size() - 1), true};
  }

  template <typename KK, typename VCtor>
  const TValue &getOrInsertLazy(KK &&Key, VCtor &&MakeV) {
    for (size_t I = 0, End = Keys.size(); I != End; ++I) {
      if (Keys[I].count(Key)) {
        return Values[I];
      }
    }
    return (*insert(std::forward<KK>(Key), std::invoke(MakeV)).first).second;
  }

  const_iterator begin() const noexcept {
    return {Values.begin(), Keys.begin()};
  }
  const_iterator end() const noexcept { return {Values.end(), Keys.end()}; }

  const_iterator find(const TKey &Ky) const {
    for (size_t I = 0, End = Keys.size(); I < End; ++I) {
      if (Keys[I].count(Ky)) {
        return getIterator(I);
      }
    }

    return end();
  }

  [[nodiscard]] inline size_t numEquivalenceClasses() const noexcept {
    return Values.size();
  }

  // Returns the size of the map, i.e., the number of equivalence classes.
  [[nodiscard]] inline size_t size() const noexcept {
    return numEquivalenceClasses();
  }

  [[nodiscard]] bool empty() const noexcept { return Values.empty(); }

  void clear() noexcept {
    Values.clear();
    Keys.clear();
  }

private:
  const_iterator getIterator(size_t I) const noexcept {
    return {std::next(Values.begin(), I), std::next(Keys.begin(), I)};
  }

  llvm::SmallVector<TValue, 2> Values;
  llvm::SmallVector<SetTy, 2> Keys;
};

} // namespace psr

#endif
