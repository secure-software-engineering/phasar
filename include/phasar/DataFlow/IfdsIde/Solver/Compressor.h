#pragma once

#include "phasar/DB/ProjectIRDBBase.h"
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
template <typename T, typename Enable = void> class Compressor;

template <typename T>
class Compressor<T, std::enable_if_t<CanEfficientlyPassByValue<T>>> {
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

  std::optional<uint32_t> getOrNull(T Elem) const {
    if (auto It = ToInt.find(Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  T operator[](size_t Idx) const noexcept {
    assert(Idx < FromInt.size());
    return FromInt[Idx];
  }

  [[nodiscard]] size_t size() const noexcept { return FromInt.size(); }
  [[nodiscard]] size_t capacity() const noexcept {
    return FromInt.capacity() +
           ToInt.getMemorySize() / sizeof(typename decltype(ToInt)::value_type);
  }

  auto begin() const noexcept { return FromInt.begin(); }
  auto end() const noexcept { return FromInt.end(); }

private:
  llvm::DenseMap<T, uint32_t> ToInt;
  llvm::SmallVector<T, 0> FromInt;
};

template <typename T>
class Compressor<T, std::enable_if_t<!CanEfficientlyPassByValue<T>>> {
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

  std::optional<uint32_t> getOrNull(const T &Elem) const {
    if (auto It = ToInt.find(&Elem); It != ToInt.end()) {
      return It->second;
    }
    return std::nullopt;
  }

  const T &operator[](size_t Idx) const noexcept {
    assert(Idx < FromInt.size());
    return FromInt[Idx];
  }

  [[nodiscard]] size_t size() const noexcept { return FromInt.size(); }
  [[nodiscard]] size_t capacity() const noexcept {
    return FromInt.size() +
           ToInt.getMemorySize() / sizeof(typename decltype(ToInt)::value_type);
  }

  auto begin() const noexcept { return FromInt.begin(); }
  auto end() const noexcept { return FromInt.end(); }

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
  llvm::DenseMap<const T *, uint32_t, DSI> ToInt;
};

struct NoneCompressor final {
  constexpr NoneCompressor() noexcept = default;

  template <typename T,
            typename = std::enable_if_t<!std::is_same_v<NoneCompressor, T>>>
  constexpr NoneCompressor(const T & /*unused*/) noexcept {}

  template <typename T>
  [[nodiscard]] decltype(auto) getOrInsert(T &&Val) const noexcept {
    return std::forward<T>(Val);
  }
  template <typename T>
  [[nodiscard]] decltype(auto) operator[](T &&Val) const noexcept {
    return std::forward<T>(Val);
  }
  void reserve(size_t /*unused*/) const noexcept {}

  [[nodiscard]] size_t size() const noexcept { return 0; }
  [[nodiscard]] size_t capacity() const noexcept { return 0; }
};

class LLVMProjectIRDB;

/// Once we have fast instruction IDs (as we already have in IntelliSecPhasar),
/// we might want to create a specialization for T/const llvm::Value * that uses
/// the IDs from the IRDB
template <typename T> struct NodeCompressorTraits {
  using type = Compressor<T>;

  static type create(const ProjectIRDBBase<LLVMProjectIRDB>
                         * /*IRDB*/) noexcept(noexcept(type())) {
    return type();
  }
};

template <typename T, typename = void> struct ValCompressorTraits {
  using type = Compressor<T>;
  using id_type = uint32_t;
};

template <typename T>
struct ValCompressorTraits<T, std::enable_if_t<CanEfficientlyPassByValue<T>>> {
  using type = NoneCompressor;
  using id_type = T;
};

} // namespace psr
