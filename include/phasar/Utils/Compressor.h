#ifndef PHASAR_UTILS_COMPRESSOR_H
#define PHASAR_UTILS_COMPRESSOR_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/SmallVector.h"

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

  std::optional<IdT> getOrNull(T Elem) const {
    if (auto It = ToInt.find(Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] bool inbounds(IdT Idx) const noexcept {
    return size_t(Idx) < FromInt.size();
  }

  T operator[](IdT Idx) const noexcept {
    assert(inbounds(Idx));
    return FromInt[size_t(Idx)];
  }

  [[nodiscard]] size_t size() const noexcept { return FromInt.size(); }
  [[nodiscard]] size_t capacity() const noexcept {
    return FromInt.capacity() +
           ToInt.getMemorySize() / sizeof(typename decltype(ToInt)::value_type);
  }

  auto begin() const noexcept { return FromInt.begin(); }
  auto end() const noexcept { return FromInt.end(); }

  void clear() noexcept {
    ToInt.clear();
    FromInt.clear();
  }

private:
  llvm::DenseMap<T, IdT> ToInt;
  llvm::SmallVector<T, 0> FromInt;
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

  /// Returns the index of the given element in the compressors storage. If the
  /// element isn't present yet, it will be added first and its index will
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

  /// Returns the index of the given element in the compressors storage. If the
  /// element isn't present yet, it will be added first and its index will
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

  /// Returns the index of the given element in the compressors storage. If the
  /// element isn't present, std::nullopt will be returned
  std::optional<IdT> getOrNull(const T &Elem) const {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] bool inbounds(IdT Idx) const noexcept {
    return size_t(Idx) < FromInt.size();
  }

  const T &operator[](IdT Idx) const noexcept {
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
