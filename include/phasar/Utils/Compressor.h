/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#ifndef PHASAR_UTILS_COMPRESSOR_H
#define PHASAR_UTILS_COMPRESSOR_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <type_traits>

namespace psr {
template <typename T, typename IdT = uint32_t, typename Enable = void>
class Compressor;

/// \brief A utility class that assigns a sequential Id to every inserted
/// object.
///
/// This specialization handles types that can be efficiently passed by value
template <typename T, typename IdT>
class Compressor<T, IdT, std::enable_if_t<CanEfficientlyPassByValue<T>>> {
public:
  void reserve(size_t Capacity) {
    assert(Capacity <= UINT32_MAX);
    ToInt.reserve(Capacity);
    FromInt.reserve(Capacity);
  }

  IdT getOrInsert(T Elem) {
    auto [It, Inserted] = ToInt.try_emplace(Elem, IdT(ToInt.size()));
    if (Inserted) {
      FromInt.push_back(Elem);
    }
    return It->second;
  }

  std::pair<IdT, bool> insert(T Elem) {
    auto [It, Inserted] = ToInt.try_emplace(Elem, IdT(ToInt.size()));
    if (Inserted) {
      FromInt.push_back(Elem);
    }
    return {It->second, Inserted};
  }

  [[nodiscard]]
  std::optional<IdT> getOrNull(T Elem) const {
    if (auto It = ToInt.find(Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] IdT get(T Elem) const {
    auto It = ToInt.find(Elem);
    assert(It != ToInt.end());
    return It->second;
  }

  [[nodiscard]] bool inbounds(IdT Idx) const noexcept {
    return FromInt.inbounds(Idx);
  }

  [[nodiscard]] T operator[](IdT Idx) const noexcept {
    assert(inbounds(Idx));
    return FromInt[Idx];
  }

  [[nodiscard]] size_t size() const noexcept { return FromInt.size(); }
  [[nodiscard]] size_t capacity() const noexcept {
    return FromInt.capacity() +
           ToInt.getMemorySize() / sizeof(typename decltype(ToInt)::value_type);
  }

  [[nodiscard]] auto begin() const noexcept { return FromInt.begin(); }
  [[nodiscard]] auto end() const noexcept { return FromInt.end(); }

  [[nodiscard]] auto enumerate() const noexcept { return FromInt.enumerate(); }

  void clear() noexcept {
    ToInt.clear();
    FromInt.clear();
  }

private:
  llvm::DenseMap<T, IdT> ToInt;
  TypedVector<IdT, T, 0> FromInt;
};

/// \brief A utility class that assigns a sequential Id to every inserted
/// object.
///
/// This specialization handles types that cannot be efficiently passed by value
template <typename T, typename IdT>
class Compressor<T, IdT, std::enable_if_t<!CanEfficientlyPassByValue<T>>> {
public:
  void reserve(size_t Capacity) {
    assert(Capacity <= UINT32_MAX);
    ToInt.reserve(Capacity);
  }

  /// Returns the index of the given element in the compressors storage. If
  /// the element isn't present yet, it will be added first and its index will
  /// then be returned.
  IdT getOrInsert(const T &Elem) {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    auto Ret = IdT(FromInt.size());
    auto *Ins = &FromInt.emplace_back(Elem);
    ToInt[Ins] = Ret;
    return Ret;
  }

  /// Returns the index of the given element in the compressors storage. If
  /// the element isn't present yet, it will be added first and its index will
  /// then be returned.
  IdT getOrInsert(T &&Elem) {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    auto Ret = IdT(FromInt.size());
    auto *Ins = &FromInt.emplace_back(std::move(Elem));
    ToInt[Ins] = Ret;
    return Ret;
  }

  std::pair<IdT, bool> insert(const T &Elem) {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return {It->second, false};
    }
    auto Ret = Id(FromInt.size());
    auto *Ins = &FromInt.emplace_back(Elem);
    ToInt[Ins] = Ret;
    return {Ret, true};
  }

  std::pair<IdT, bool> insert(T &&Elem) {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return {It->second, false};
    }
    auto Ret = Id(FromInt.size());
    auto *Ins = &FromInt.emplace_back(std::move(Elem));
    ToInt[Ins] = Ret;
    return {Ret, true};
  }

  /// Returns the index of the given element in the compressors storage. If
  /// the element isn't present, std::nullopt will be returned
  [[nodiscard]] std::optional<IdT> getOrNull(const T &Elem) const {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] IdT get(const T &Elem) const {
    auto It = ToInt.find(&Elem);
    assert(It != ToInt.end());
    return It->second;
  }

  [[nodiscard]] bool inbounds(IdT Idx) const noexcept {
    return size_t(Idx) < FromInt.size();
  }

  [[nodiscard]] const T &operator[](IdT Idx) const noexcept {
    assert(inbounds(Idx));
    return FromInt[size_t(Idx)];
  }

  [[nodiscard]] size_t size() const noexcept { return FromInt.size(); }
  [[nodiscard]] size_t capacity() const noexcept {
    return FromInt.size() +
           ToInt.getMemorySize() / sizeof(typename decltype(ToInt)::value_type);
  }

  auto begin() const noexcept { return FromInt.begin(); }
  auto end() const noexcept { return FromInt.end(); }

  [[nodiscard]] auto enumerate() const noexcept {
    return llvm::map_range(llvm::enumerate(FromInt),
                           [](const auto &IndexAndVal) {
                             return std::pair<IdT, const T &>{
                                 IdT(IndexAndVal.index()), IndexAndVal.value()};
                           });
  }

  void clear() noexcept {
    ToInt.clear();
    FromInt.clear();
  }

private:
  struct DSI : llvm::DenseMapInfo<const T *> {
    static auto getHashValue(const T *Elem) noexcept {
      assert(Elem != nullptr);
      if constexpr (has_llvm_dense_map_info<T>) {
        return llvm::DenseMapInfo<T>::getHashValue(*Elem);
      } else {
        return std::hash<T>{}(*Elem);
      }
    }
    static auto isEqual(const T *LHS, const T *RHS) noexcept {
      if (LHS == RHS) {
        return true;
      }
      if (LHS == DSI::getEmptyKey() || LHS == DSI::getTombstoneKey() ||
          RHS == DSI::getEmptyKey() || RHS == DSI::getTombstoneKey()) {
        return false;
      }
      if constexpr (has_llvm_dense_map_info<T>) {
        return llvm::DenseMapInfo<T>::isEqual(*LHS, *RHS);
      } else {
        return *LHS == *RHS;
      }
    }
  };

  std::deque<T> FromInt;
  llvm::DenseMap<const T *, IdT, DSI> ToInt;
};

} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_SOLVER_COMPRESSOR_H
