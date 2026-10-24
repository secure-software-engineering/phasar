/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_HASHUTILS_H
#define PHASAR_UTILS_HASHUTILS_H

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"

#include <cstddef>
#include <utility>

namespace psr {
struct PairHash {
  template <typename T, typename U>
  size_t operator()(const std::pair<T, U> &Pair) const noexcept {
    return llvm::DenseMapInfo<std::pair<T, U>>::getHashValue(Pair);
  }
};

template <typename T> struct DefaultHash {
  [[nodiscard]] size_t operator()(const T &Value) const noexcept {
    if constexpr (is_llvm_hashable_v<T>) {
      using llvm::hash_value;
      return hash_value(Value);
    } else {
      return std::hash<T>{}(Value);
    }
  }
};
} // namespace psr

#endif // PHASAR_UTILS_HASHUTILS_H
