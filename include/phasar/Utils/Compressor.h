/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_COMPRESSOR_H
#define PHASAR_PHASARLLVM_UTILS_COMPRESSOR_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <type_traits>

namespace psr {

namespace detail {
template <typename T, typename Enable = void> class CompressorBase;

template <typename T>
class CompressorBase<T, std::enable_if_t<CanEfficientlyPassByValue<T> &&
                                         HasLLVMDenseMapInfo<T>>> {
public:
  void reserve(size_t Capacity) {
    assert(Capacity <= UINT32_MAX);
    ToInt.reserve(Capacity);
    FromInt.reserve(Capacity);
  }

  uint32_t getOrInsert(T Elem) {
    auto [It, Inserted] = ToInt.try_emplace(Elem, ToInt.size());
    if (Inserted) {
      FromInt.push_back(Elem);
    }
    return It->second;
  }

  [[nodiscard]] std::optional<uint32_t> getOrNull(T Elem) const noexcept {
    if (auto It = ToInt.find(Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] T operator[](size_t Idx) const noexcept {
    assert(Idx < FromInt.size());
    return FromInt[Idx];
  }

  [[nodiscard]] size_t size() const noexcept { return FromInt.size(); }
  [[nodiscard]] size_t capacity() const noexcept {
    return FromInt.capacity() +
           ToInt.getMemorySize() / sizeof(typename decltype(ToInt)::value_type);
  }

  [[nodiscard]] auto begin() const noexcept { return FromInt.begin(); }
  [[nodiscard]] auto end() const noexcept { return FromInt.end(); }

private:
  llvm::DenseMap<T, uint32_t> ToInt{};
  llvm::SmallVector<T, 0> FromInt{};
};

template <typename T>
class CompressorBase<T, std::enable_if_t<!CanEfficientlyPassByValue<T> ||
                                         !HasLLVMDenseMapInfo<T>>> {
public:
  void reserve(size_t Capacity) {
    assert(Capacity <= UINT32_MAX);
    ToInt.reserve(Capacity);
  }

  uint32_t getOrInsert(const T &Elem) {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    auto Ret = FromInt.size();
    auto *Ins = &FromInt.emplace_back(Elem);
    ToInt[Ins] = Ret;
    return Ret;
  }

  uint32_t getOrInsert(T &&Elem) {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    auto Ret = FromInt.size();
    auto *Ins = &FromInt.emplace_back(std::move(Elem));
    ToInt[Ins] = Ret;
    return Ret;
  }

  [[nodiscard]] std::optional<uint32_t>
  getOrNull(const T &Elem) const noexcept {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] const T &operator[](size_t Idx) const noexcept {
    assert(Idx < FromInt.size());
    return FromInt[Idx];
  }

  [[nodiscard]] size_t size() const noexcept { return FromInt.size(); }
  [[nodiscard]] size_t capacity() const noexcept {
    return FromInt.size() +
           ToInt.getMemorySize() / sizeof(decltype(ToInt)::value_type);
  }

  [[nodiscard]] auto begin() const noexcept { return FromInt.begin(); }
  [[nodiscard]] auto end() const noexcept { return FromInt.end(); }

private:
  struct DSI : llvm::DenseMapInfo<const T *> {
    static auto getHashValue(const T *Elem) noexcept {
      assert(Elem != nullptr);
      if constexpr (HasLLVMDenseMapInfo<T>) {
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
      if constexpr (HasLLVMDenseMapInfo<T>) {
        return llvm::DenseMapInfo<T>::isEqual(*LHS, *RHS);
      } else {
        return *LHS == *RHS;
      }
    }
  };

  std::deque<T> FromInt{};
  llvm::DenseMap<const T *, uint32_t, DSI> ToInt{};
};
} // namespace detail

template <typename T> class Compressor : public detail::CompressorBase<T> {
public:
  template <typename Iter>
  [[nodiscard]] llvm::SmallBitVector getOrInsertSet(Iter Begin, Iter End) {
    llvm::SmallBitVector Ret;
    while (Begin != End) {
      auto Idx = getOrInsert(*Begin);
      if (Ret.size() <= Idx) {
        Ret.resize(Idx + 1);
      }
      Ret.set(Idx);
      ++Begin;
    }
    return Ret;
  }

  template <typename RangeT>
  [[nodiscard]] llvm::SmallBitVector getOrInsertSet(RangeT &&Rng) {
    using std::begin;
    using std::end;
    return getOrInsertSet(begin(Rng), end(Rng));
  }

private:
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_COMPRESSOR_H
