#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <execution>
#include <utility>

namespace psr {

namespace array_set::detail {
template <typename T>
[[nodiscard]] llvm::ArrayRef<T> dropFront(llvm::ArrayRef<T> AR, size_t Drop) {
  if (Drop >= AR.size()) {
    return {};
  }

  return AR.drop_front(Drop);
}
} // namespace array_set::detail

template <typename T,
          unsigned SmallSize =
              llvm::CalculateSmallVectorDefaultInlinedElements<T>::value>
class ArraySet {
public:
  ArraySet() noexcept = default;

  bool insert(T Value)
    requires(psr::CanEfficientlyPassByValue<T>)
  {
    using array_set::detail::dropFront;
    if (isContained(Vecs[InsertIndex], Value) ||
        isContained(dropFront<T>(Vecs[1 - InsertIndex], IterIdx + 1), Value)) {

      return false;
    }

    Vecs[InsertIndex].push_back(Value);
    return true;
  }

  bool insert(auto &&Value)
    requires(psr::CanEfficientlyPassByValue<T>)
  {
    using array_set::detail::dropFront;
    if (isContained(Vecs[InsertIndex], Value) ||
        isContained(dropFront<T>(Vecs[1 - InsertIndex], IterIdx + 1), Value)) {
      return false;
    }

    Vecs[InsertIndex].push_back(PSR_FWD(Value));
    return true;
  }

  [[nodiscard]] bool empty() const noexcept {
    return Vecs[0].empty() && Vecs[1].empty();
  }

  template <typename BodyT> void foreach (BodyT Body) {
    assert(IterIdx == 0);
    do {
      auto QIndex = 1 - InsertIndex;
      auto &Q = Vecs[QIndex];
      for (; IterIdx != Q.size(); ++IterIdx) {
        std::invoke(Body, std::move(Q[IterIdx]));
      }
      Vecs[QIndex].clear();
      IterIdx = 0;
      InsertIndex = QIndex;
    } while (!Vecs[1 - InsertIndex].empty());
  }

  [[nodiscard]] size_t getMemorySize() const noexcept {
    return Vecs[0].capacity_in_bytes() + Vecs[1].capacity_in_bytes();
  }

private:
  static bool isContained(llvm::ArrayRef<T> Data, ByConstRef<T> Val) noexcept {
    auto It = std::find(std::execution::unseq, Data.begin(), Data.end(), Val);
    return It != Data.end();
  }

  std::array<llvm::SmallVector<T, SmallSize>, 2> Vecs;
  size_t InsertIndex = 0;
  size_t IterIdx = 0;
};
} // namespace psr
