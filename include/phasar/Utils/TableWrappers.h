/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_TABLEWRAPPERS
#define PHASAR_UTILS_TABLEWRAPPERS

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/EmptyBaseOptimizationUtils.h"
#include "phasar/Utils/MemoryResource.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator_range.h"

#include <functional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace psr {

template <typename T, typename = size_t>
struct has_getHashCode : std::false_type {};
template <typename T>
struct has_getHashCode<T, decltype(std::declval<const T>().getHashCode())>
    : std::true_type {};

template <typename T, typename = size_t>
struct has_std_hash : std::false_type {};
template <typename T>
struct has_std_hash<T, decltype(std::hash<T>{}(std::declval<const T>))>
    : std::true_type {};

namespace detail {
template <typename K, typename V> struct DummyTransform {
  ByConstRef<V> Value;

  // 'const' needed for decltype within llvm::mapped_iterator
  // NOLINTNEXTLINE(readability-const-return-type)
  const std::pair<K, V> operator()(ByConstRef<K> Key) const {
    return {Key, Value};
  }
};

template <typename T> using CellVecSmallVectorTy = llvm::SmallVector<T, 8>;

template <typename K> struct Hasher {
  size_t operator()(ByConstRef<K> Key) const noexcept {
    if constexpr (has_getHashCode<K>::value) {
      return Key.getHashCode();
    } else {
      return std::hash<K>{}(Key);
    }
  }
};

} // namespace detail

template <typename K, typename V> class UnorderedTable1d {
  using Hasher = detail::Hasher<K>;

public:
  using value_type = std::pair<K, V>;
  using iterator = typename std::unordered_map<K, V, Hasher>::iterator;
  using const_iterator =
      typename std::unordered_map<K, V, Hasher>::const_iterator;

  UnorderedTable1d() noexcept = default;

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.count(Key);
  }

  V &getOrCreate(K Key) { return Map[std::move(Key)]; }

  auto insert(K Key, V Value) {
    return Map.try_emplace(std::move(Key), std::move(Value));
  }

  template <typename VV = V,
            typename = std::enable_if_t<CanEfficientlyPassByValue<VV>>>
  V getOr(ByConstRef<K> Key, V Or) const {
    if (auto It = Map.find(Key); It != Map.end()) {
      return It->second;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, const V &Or) const {
    if (auto It = Map.find(Key); It != Map.end()) {
      return It->second;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, V &&Or) const = delete;

  const_iterator find(ByConstRef<K> Key) const { return Map.find(Key); }

  void erase(ByConstRef<K> Key) { Map.erase(Key); }

  void erase(const_iterator It) { Map.erase(It); }

  auto cells() noexcept { return llvm::make_range(Map.begin(), Map.end()); }
  auto cells() const noexcept {
    return llvm::make_range(Map.begin(), Map.end());
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    Ret.insert(Ret.end(), Map.begin(), Map.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      std::pair<std::invoke_result_t<ResultProjection, K>, V>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        std::pair<std::invoke_result_t<ResultProjection, K>, V>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem.first) == Of) {
        Ret.emplace_back(std::invoke(ResProj, Elem.first), Elem.second);
      }
    }
    return Ret;
  }

  void clear() {
    std::unordered_map<K, V, Hasher> Empty{};
    swap(Empty, Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.bucket_count() * sizeof(void *) +
           Map.size() * sizeof(std::tuple<void *, void *,
                                          typename decltype(Map)::value_type>);
  }

private:
  std::unordered_map<K, V, Hasher> Map;
};

template <typename K> class UnorderedTable1d<K, EmptyType> {
  using Hasher = detail::Hasher<K>;

public:
  using value_type = DummyPair<K>;
  using iterator = typename std::unordered_set<DummyPair<K>>::iterator;
  using const_iterator =
      typename std::unordered_set<DummyPair<K>>::const_iterator;

  UnorderedTable1d() noexcept = default;

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.count({Key, {}});
  }

  EmptyType getOrCreate(K Key) {
    Map.insert({std::move(Key), {}});
    return {};
  }

  auto insert(K Key) { return Map.insert({std::move(Key), {}}); }
  auto insert(K Key, EmptyType /*Value*/) { return insert(std::move(Key)); }

  const_iterator find(ByConstRef<K> Key) const { return Map.find({Key, {}}); }

  void erase(ByConstRef<K> Key) { Map.erase({Key, {}}); }

  void erase(const_iterator It) { Map.erase(It); }

  auto cells() noexcept { return llvm::make_range(Map.begin(), Map.end()); }
  auto cells() const noexcept {
    return llvm::make_range(Map.begin(), Map.end());
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    Ret.insert(Ret.end(), Map.begin(), Map.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      DummyPair<std::invoke_result_t<ResultProjection, K>>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        DummyPair<std::invoke_result_t<ResultProjection, K>>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem.first) == Of) {
        Ret.push_back({std::invoke(ResProj, Elem.first), {}});
      }
    }
    return Ret;
  }

  void clear() {
    std::unordered_set<DummyPair<K>, Hasher> Empty{};
    swap(Empty, Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.bucket_count() * sizeof(void *) +
           Map.size() * sizeof(std::tuple<void *, void *,
                                          typename decltype(Map)::value_type>);
  }

private:
  std::unordered_set<DummyPair<K>, Hasher> Map;
};

template <typename K, typename V> class DummyUnorderedTable1d {
  struct Hasher {
    size_t operator()(ByConstRef<K> Key) const noexcept {
      if constexpr (has_getHashCode<K>::value) {
        return Key.getHashCode();
      } else {
        return std::hash<K>{}(Key);
      }
    }
  };

public:
  using value_type = std::pair<K, V>;
  using iterator = llvm::mapped_iterator<
      typename std::unordered_set<K, Hasher>::const_iterator,
      detail::DummyTransform<K, V>>;
  using const_iterator = iterator;

  explicit DummyUnorderedTable1d(V Value) noexcept(
      std::is_nothrow_move_constructible_v<V>)
      : Value(std::move(Value)) {}

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.count(Key);
  }

  const V &getOrCreate(K Key) {
    Map.insert(std::move(Key));
    return Value;
  }

  template <typename VV = V,
            typename = std::enable_if_t<CanEfficientlyPassByValue<VV>>>
  V getOr(ByConstRef<K> Key, V Or) const {
    if (Map.count(Key)) {
      return Value;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, const V &Or) const {
    if (Map.count(Key)) {
      return Value;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, V &&Or) const = delete;

  const_iterator find(ByConstRef<K> Key) const {
    return llvm::map_iterator(Map.find(Key),
                              detail::DummyTransform<K, V>{Value});
  }

  void erase(ByConstRef<K> Key) { Map.erase(Key); }

  void erase(iterator It) { Map.erase(It.getCurrent()); }

  auto cells() const noexcept {
    return llvm::map_range(llvm::make_range(Map.begin(), Map.end()),
                           detail::DummyTransform<K, V>{Value});
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    auto Cells = cells();
    Ret.insert(Ret.end(), Cells.begin(), Cells.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.emplace_back(Elem, Value);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      std::pair<std::invoke_result_t<ResultProjection, K>, V>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        std::pair<std::invoke_result_t<ResultProjection, K>, V>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem) == Of) {
        Ret.emplace_back(std::invoke(ResProj, Elem), Value);
      }
    }
    return Ret;
  }

  void clear() {
    std::unordered_set<K, Hasher> Empty{};
    swap(Empty, Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.bucket_count() * sizeof(void *) +
           Map.size() * sizeof(std::tuple<void *, void *,
                                          typename decltype(Map)::value_type>);
  }

private:
  std::unordered_set<K, Hasher> Map;
  V Value{};
};

template <typename T> class UnorderedSet {
public:
  using value_type = T;
  UnorderedSet() noexcept = default;

  void reserve(size_t Capacity) { Set.reserve(Capacity); }

  auto insert(T Val) { return Set.insert(std::move(Val)); }

  bool contains(ByConstRef<T> Val) const noexcept { return Set.count(Val); }

  bool erase(ByConstRef<T> Val) { return Set.erase(Val); }

  auto begin() const noexcept { return Set.begin(); }
  auto end() const noexcept { return Set.end(); }

  void
  clear() noexcept(std::is_nothrow_default_constructible_v<decltype(Set)>) {
    std::unordered_set<T, Hasher> Empty{};
    swap(Set, Empty);
  }

  auto cells() const noexcept { return llvm::make_range(begin(), end()); }

  detail::CellVecSmallVectorTy<T> cellVec() const {
    detail::CellVecSmallVectorTy<T> Ret;
    Ret.reserve(Set.size());
    Ret.insert(Ret.end(), Set.begin(), Set.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<T> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<T> Ret;
    for (const auto &Elem : Set) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<std::invoke_result_t<ResultProjection, T>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, T>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<std::invoke_result_t<ResultProjection, T>> Ret;
    for (const auto &Elem : Set) {
      if (std::invoke(Proj, Elem) == Of) {
        Ret.push_back(std::invoke(ResProj, Elem));
      }
    }
    return Ret;
  }

  [[nodiscard]] size_t size() const noexcept { return Set.size(); }
  [[nodiscard]] bool empty() const noexcept { return Set.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Set.bucket_count() * sizeof(void *) +
           Set.size() * sizeof(std::tuple<void *, void *,
                                          typename decltype(Set)::value_type>);
  }

private:
  struct Hasher {
    size_t operator()(ByConstRef<T> Key) const noexcept {
      if constexpr (has_getHashCode<T>::value) {
        return Key.getHashCode();
      } else {
        return std::hash<T>{}(Key);
      }
    }
  };

  std::unordered_set<T, Hasher> Set;
};

template <typename K, typename V> class DenseTable1d {

public:
  using value_type = std::pair<K, V>;
  using iterator = typename llvm::DenseMap<K, V>::iterator;
  using const_iterator = typename llvm::DenseMap<K, V>::const_iterator;

  DenseTable1d() noexcept = default;
  /// Dummy ctor for compatibility with UnorderedTable1d
  template <typename Allocator>
  explicit DenseTable1d(Allocator /*Alloc*/) noexcept {}

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.count(Key);
  }

  V &getOrCreate(K Key) { return Map[std::move(Key)]; }

  auto insert(K Key, V Value) {
    return Map.try_emplace(std::move(Key), std::move(Value));
  }

  template <typename VV = V,
            typename = std::enable_if_t<CanEfficientlyPassByValue<VV>>>
  V getOr(ByConstRef<K> Key, V Or) const {
    if (auto It = Map.find(Key); It != Map.end()) {
      return It->second;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, const V &Or) const {
    if (auto It = Map.find(Key); It != Map.end()) {
      return It->second;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, V &&Or) const = delete;

  const_iterator find(ByConstRef<K> Key) const { return Map.find(Key); }

  void erase(ByConstRef<K> Key) { Map.erase(Key); }
  void erase(iterator It) { Map.erase(It); }

  auto cells() noexcept { return llvm::make_range(Map.begin(), Map.end()); }
  auto cells() const noexcept {
    return llvm::make_range(Map.begin(), Map.end());
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    Ret.insert(Ret.end(), Map.begin(), Map.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      std::pair<std::invoke_result_t<ResultProjection, K>, V>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        std::pair<std::invoke_result_t<ResultProjection, K>, V>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem.first) == Of) {
        Ret.emplace_back(std::invoke(ResProj, Elem.first), Elem.second);
      }
    }
    return Ret;
  }

  void clear() noexcept {
    llvm::DenseMap<K, V> Empty;
    Empty.swap(Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.getMemorySize();
  }

private:
  llvm::DenseMap<K, V> Map;
};

template <typename K> class DenseTable1d<K, EmptyType> {

public:
  using value_type = DummyPair<K>;
  using iterator = typename llvm::DenseSet<DummyPair<K>>::iterator;
  using const_iterator = typename llvm::DenseSet<DummyPair<K>>::const_iterator;

  DenseTable1d() noexcept = default;
  /// Dummy ctor for compatibility with UnorderedTable1d
  template <typename Allocator>
  explicit DenseTable1d(Allocator /*Alloc*/) noexcept {}

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.count({Key, {}});
  }

  EmptyType getOrCreate(K Key) {
    Map.insert({std::move(Key), {}});
    return {};
  }

  auto insert(K Key) { return Map.insert({std::move(Key), {}}); }
  auto insert(K Key, EmptyType /*Value*/) { return insert(std::move(Key)); }

  const_iterator find(ByConstRef<K> Key) const { return Map.find({Key, {}}); }

  void erase(ByConstRef<K> Key) { Map.erase({Key, {}}); }
  void erase(iterator It) { Map.erase(It); }

  auto cells() noexcept { return llvm::make_range(Map.begin(), Map.end()); }
  auto cells() const noexcept {
    return llvm::make_range(Map.begin(), Map.end());
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    Ret.insert(Ret.end(), Map.begin(), Map.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      DummyPair<std::invoke_result_t<ResultProjection, K>>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        DummyPair<std::invoke_result_t<ResultProjection, K>>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem.first) == Of) {
        Ret.push_back({std::invoke(ResProj, Elem.first), {}});
      }
    }
    return Ret;
  }

  void clear() noexcept {
    llvm::DenseSet<DummyPair<K>> Empty;
    Empty.swap(Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.getMemorySize();
  }

private:
  llvm::DenseSet<DummyPair<K>> Map;
};

template <typename K, typename V, unsigned N> class SmallDenseTable1d {

public:
  using container_type = llvm::SmallDenseMap<K, V, N>;
  using value_type = std::pair<K, V>;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  SmallDenseTable1d() noexcept = default;
  /// Dummy ctor for compatibility with UnorderedTable1d
  template <typename Allocator>
  explicit SmallDenseTable1d(Allocator /*Alloc*/) noexcept {}

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.count(Key);
  }

  V &getOrCreate(K Key) { return Map[std::move(Key)]; }

  auto insert(K Key, V Value) {
    return Map.try_emplace(std::move(Key), std::move(Value));
  }

  template <typename VV = V,
            typename = std::enable_if_t<CanEfficientlyPassByValue<VV>>>
  V getOr(ByConstRef<K> Key, V Or) const {
    if (auto It = Map.find(Key); It != Map.end()) {
      return It->second;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, const V &Or) const {
    if (auto It = Map.find(Key); It != Map.end()) {
      return It->second;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, V &&Or) const = delete;

  const_iterator find(ByConstRef<K> Key) const { return Map.find(Key); }

  void erase(ByConstRef<K> Key) { Map.erase(Key); }
  void erase(iterator It) { Map.erase(It); }

  auto cells() noexcept { return llvm::make_range(Map.begin(), Map.end()); }
  auto cells() const noexcept {
    return llvm::make_range(Map.begin(), Map.end());
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    Ret.insert(Ret.end(), Map.begin(), Map.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      std::pair<std::invoke_result_t<ResultProjection, K>, V>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        std::pair<std::invoke_result_t<ResultProjection, K>, V>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem.first) == Of) {
        Ret.emplace_back(std::invoke(ResProj, Elem.first), Elem.second);
      }
    }
    return Ret;
  }

  void clear() noexcept {
    container_type Empty;
    Empty.swap(Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.getMemorySize();
  }

private:
  container_type Map;
};

template <typename K, unsigned N> class SmallDenseTable1d<K, EmptyType, N> {

public:
  using container_type = llvm::SmallDenseSet<DummyPair<K>>;
  using value_type = DummyPair<K>;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  SmallDenseTable1d() noexcept = default;
  /// Dummy ctor for compatibility with UnorderedTable1d
  template <typename Allocator>
  explicit SmallDenseTable1d(Allocator /*Alloc*/) noexcept {}

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.count({Key, {}});
  }

  EmptyType getOrCreate(K Key) {
    Map.insert({std::move(Key), {}});
    return {};
  }

  auto insert(K Key) { return Map.insert({std::move(Key), {}}); }
  auto insert(K Key, EmptyType /*Value*/) { return insert(std::move(Key)); }

  const_iterator find(ByConstRef<K> Key) const { return Map.find({Key, {}}); }

  void erase(ByConstRef<K> Key) { Map.erase({Key, {}}); }
  void erase(iterator It) { Map.erase(It); }

  auto cells() noexcept { return llvm::make_range(Map.begin(), Map.end()); }
  auto cells() const noexcept {
    return llvm::make_range(Map.begin(), Map.end());
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    Ret.insert(Ret.end(), Map.begin(), Map.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      DummyPair<std::invoke_result_t<ResultProjection, K>>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        DummyPair<std::invoke_result_t<ResultProjection, K>>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem.first) == Of) {
        Ret.push_back({std::invoke(ResProj, Elem.first), {}});
      }
    }
    return Ret;
  }

  void clear() noexcept {
    container_type Empty;
    Empty.swap(Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.getMemorySize();
  }

private:
  container_type Map;
};

/// A set that appears as map mapping to a constant
template <typename K, typename V> class DummyDenseTable1d {

public:
  using value_type = typename std::pair<K, V>;
  using iterator =
      typename llvm::mapped_iterator<typename llvm::DenseSet<K>::const_iterator,
                                     detail::DummyTransform<K, V>>;
  using const_iterator = iterator;

  DummyDenseTable1d() noexcept = default;
  /// Dummy ctor for compatibility with UnorderedTable1d
  template <typename Allocator>
  explicit DummyDenseTable1d(Allocator /*Alloc*/) noexcept {}
  template <typename Allocator>
  explicit DummyDenseTable1d(Allocator /*Alloc*/, V Value) noexcept
      : Value(std::move(Value)) {}

  void reserve(size_t Capacity) { Map.reserve(Capacity); }

  [[nodiscard]] bool contains(ByConstRef<K> Key) const noexcept {
    return Map.contains(Key);
  }

  const V &getOrCreate(K Key) {
    Map.insert(std::move(Key));
    return Value;
  }

  template <typename VV = V,
            typename = std::enable_if_t<CanEfficientlyPassByValue<VV>>>
  V getOr(ByConstRef<K> Key, V Or) const {
    if (Map.contains(Key)) {
      return Value;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, const V &Or) const {
    if (Map.contains(Key)) {
      return Value;
    }
    return Or;
  }

  template <typename VV = V,
            typename = std::enable_if_t<!CanEfficientlyPassByValue<VV>>>
  const V &getOr(ByConstRef<K> Key, V &&Or) const = delete;

  const_iterator find(ByConstRef<K> Key) const {
    return llvm::map_iterator(Map.find(Key),
                              detail::DummyTransform<K, V>{Value});
  }

  void erase(ByConstRef<K> Key) { Map.erase(Key); }
  void erase(iterator It) { Map.erase(It.getCurrent()); }

  auto cells() const noexcept {
    return llvm::map_range(llvm::make_range(Map.begin(), Map.end()),
                           detail::DummyTransform<K, V>{Value});
  }

  detail::CellVecSmallVectorTy<value_type> cellVec() const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    Ret.reserve(Map.size());
    auto Cells = cells();
    Ret.insert(Ret.end(), Cells.begin(), Cells.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<value_type> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<value_type> Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Filter, Elem)) {
        Ret.emplace_back(Elem, Value);
      }
    }
    return Ret;
  }

  template <typename Projection, typename ResultProjection = IdentityFn>
  detail::CellVecSmallVectorTy<
      std::pair<std::invoke_result_t<ResultProjection, K>, V>>
  allOf(Projection Proj, ByConstRef<std::invoke_result_t<Projection, K>> Of,
        ResultProjection ResProj = {}) const {
    detail::CellVecSmallVectorTy<
        std::pair<std::invoke_result_t<ResultProjection, K>, V>>
        Ret;
    for (const auto &Elem : Map) {
      if (std::invoke(Proj, Elem) == Of) {
        Ret.emplace_back(std::invoke(ResProj, Elem), Value);
      }
    }
    return Ret;
  }

  void clear() noexcept {
    llvm::DenseSet<K> Empty;
    Empty.swap(Map);
  }

  [[nodiscard]] size_t size() const noexcept { return Map.size(); }
  [[nodiscard]] bool empty() const noexcept { return Map.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Map.getMemorySize();
  }

  [[nodiscard]] iterator begin() const noexcept {
    return llvm::map_iterator(Map.begin(), detail::DummyTransform<K, V>{Value});
  }
  [[nodiscard]] iterator end() const noexcept {
    return llvm::map_iterator(Map.end(), detail::DummyTransform<K, V>{Value});
  }

private:
  llvm::DenseSet<K> Map;
  V Value{};
};

template <typename T, typename DSI = llvm::DenseMapInfo<T>> class DenseSet {
public:
  using value_type = T;
  DenseSet() noexcept = default;
  template <typename Allocator> DenseSet(Allocator /*Alloc*/) noexcept {}

  void reserve(size_t Capacity) { Set.reserve(Capacity); }

  auto insert(T Val) { return Set.insert(std::move(Val)); }

  bool contains(ByConstRef<T> Val) const noexcept { return Set.count(Val); }

  bool erase(ByConstRef<T> Val) { return Set.erase(Val); }

  auto begin() const noexcept { return Set.begin(); }
  auto end() const noexcept { return Set.end(); }

  void clear() noexcept {
    llvm::DenseSet<T, DSI> Empty;
    Empty.swap(Set);
  }

  auto cells() const noexcept { return llvm::make_range(begin(), end()); }

  detail::CellVecSmallVectorTy<T> cellVec() const {
    detail::CellVecSmallVectorTy<T> Ret;
    Ret.reserve(Set.size());
    Ret.insert(Ret.end(), Set.begin(), Set.end());
    return Ret;
  }

  template <typename FilterFn>
  detail::CellVecSmallVectorTy<T> cellVec(FilterFn Filter) const {
    detail::CellVecSmallVectorTy<T> Ret;
    for (const auto &Elem : Set) {
      if (std::invoke(Filter, Elem)) {
        Ret.push_back(Elem);
      }
    }
    return Ret;
  }

  template <typename Projection>
  detail::CellVecSmallVectorTy<std::invoke_result_t<Projection, T>>
  allOf(Projection Proj,
        ByConstRef<std::invoke_result_t<Projection, T>> Of) const {
    detail::CellVecSmallVectorTy<std::invoke_result_t<Projection, T>> Ret;
    for (const auto &Elem : Set) {
      auto &&ProjKey = std::invoke(Proj, Elem);
      if (ProjKey == Of) {
        Ret.push_back(std::forward<decltype(ProjKey)>(ProjKey));
      }
    }
    return Ret;
  }

  [[nodiscard]] size_t size() const noexcept { return Set.size(); }
  [[nodiscard]] bool empty() const noexcept { return Set.empty(); }

  [[nodiscard]] size_t getApproxSizeInBytes() const noexcept {
    return Set.getMemorySize();
  }

private:
  llvm::DenseSet<T, DSI> Set;
};

} // namespace psr

#endif // PHASAR_UTILS_TABLEWRAPPERS
