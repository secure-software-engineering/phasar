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

#include "llvm/ADT/DenseMapInfo.h"

#include <cstddef>
#include <utility>

namespace psr {
struct PairHash {
  template <typename T, typename U>
  size_t operator()(const std::pair<T, U> &Pair) const noexcept {
    return llvm::DenseMapInfo<std::pair<T, U>>::getHashValue(Pair);
  }
};
} // namespace psr

#endif // PHASAR_UTILS_HASHUTILS_H
