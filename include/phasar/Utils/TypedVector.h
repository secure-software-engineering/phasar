/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#ifndef PHASAR_UTILS_TYPEDVECTOR_H
#define PHASAR_UTILS_TYPEDVECTOR_H

#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace psr {

/// Wraps a llvm::SmallVector, allowing index-based access by IdT, instead of
/// size_t.
///
/// \tparam IdT The index-type that should be used for operator[]. Must be
/// losslessly convertible from and to size_t.
/// \tparam ValueT The usual value_type of SmallVector.
/// \tparam SmallSize The size of the inline-storage of SmallVector (default:
/// 0)
template <typename IdT, typename ValueT, unsigned SmallSize = 0>
class TypedVector {
public:
  TypedVector() noexcept = default;
  TypedVector(std::initializer_list<ValueT> IList) : Vec(IList) {}
  TypedVector(size_t Size) : Vec(Size) {}
  TypedVector(size_t Size, ByConstRef<ValueT> Default) : Vec(Size, Default) {};

  template <typename Iter>
  explicit TypedVector(Iter From, Iter To)
      : Vec(std::move(From), std::move(To)) {}

  explicit TypedVector(llvm::ArrayRef<ValueT> Arr)
      : Vec(Arr.begin(), Arr.end()) {}

  void reserve(size_t Capa) { Vec.reserve(Capa); }

  void resize(size_t Sz) { Vec.resize(Sz); }

  void resize(size_t Sz, ByConstRef<ValueT> Val) { Vec.resize(Sz, Val); }

  [[nodiscard]] bool empty() const noexcept { return Vec.empty(); }
  [[nodiscard]] bool any() const noexcept { return !Vec.empty(); }
  [[nodiscard]] size_t size() const noexcept { return Vec.size(); }
  [[nodiscard]] size_t capacity() const noexcept { return Vec.capacity(); }

  [[nodiscard]] bool inbounds(IdT Id) const noexcept {
    return size_t(Id) < size();
  }

  [[nodiscard]] const ValueT &operator[](IdT Id) const & {
    assert(inbounds(Id));
    return Vec[size_t(Id)];
  }

  [[nodiscard]] ValueT &operator[](IdT Id) & {
    assert(inbounds(Id));
    return Vec[size_t(Id)];
  }

  [[nodiscard]] ValueT operator[](IdT Id) && {
    assert(inbounds(Id));
    return std::move(Vec[size_t(Id)]);
  }

  [[nodiscard]] auto begin() noexcept { return Vec.begin(); }
  [[nodiscard]] auto end() noexcept { return Vec.end(); }

  [[nodiscard]] auto begin() const noexcept { return Vec.begin(); }
  [[nodiscard]] auto end() const noexcept { return Vec.end(); }

  template <typename... ArgsT> ValueT &emplace_back(ArgsT &&...Args) {
    return Vec.emplace_back(std::forward<ArgsT>(Args)...);
  }

  void push_back(ByConstRef<ValueT> Val) { Vec.push_back(Val); }

  template <typename T = ValueT>
  std::enable_if_t<!CanEfficientlyPassByValue<T>> push_back(ValueT &&Val) {
    Vec.push_back(std::move(Val));
  }

  void pop_back() { Vec.pop_back(); }
  [[nodiscard]] ValueT pop_back_val() { return Vec.pop_back_val(); }

  [[nodiscard]] bool operator==(const TypedVector &Other) const noexcept {
    return Vec == Other.Vec;
  }
  [[nodiscard]] bool operator!=(const TypedVector &Other) const noexcept {
    return !(*this == Other);
  }

  [[nodiscard]] llvm::ArrayRef<ValueT> asRef() const & noexcept { return Vec; }
  [[nodiscard]] llvm::ArrayRef<ValueT> asRef() && noexcept = delete;

  [[nodiscard]] llvm::ArrayRef<ValueT>
  // NOLINTNEXTLINE(readability-identifier-naming)
  drop_front(size_t Offs) const & noexcept {
    return asRef().drop_front(Offs);
  }
  [[nodiscard]] llvm::ArrayRef<ValueT>
  drop_front(size_t Offs) && noexcept = delete;

  [[nodiscard]] auto enumerate() const noexcept {
    return llvm::map_range(llvm::enumerate(Vec), [](const auto &IndexAndVal) {
      return std::pair<IdT, ByConstRef<ValueT>>{IdT(IndexAndVal.index()),
                                                IndexAndVal.value()};
    });
  }
  [[nodiscard]] auto enumerate() noexcept {
    return llvm::map_range(llvm::enumerate(Vec), [](auto &IndexAndVal) {
      return std::pair<IdT, ValueT &>{IdT(IndexAndVal.index()),
                                      IndexAndVal.value()};
    });
  }

private:
  llvm::SmallVector<ValueT, SmallSize> Vec{};
};
} // namespace psr

#endif
