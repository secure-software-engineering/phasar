/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#ifndef PHASAR_UTILS_BITSETHASH_H
#define PHASAR_UTILS_BITSETHASH_H

#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallBitVector.h"

#include <cstdint>

namespace psr {
namespace detail {
[[nodiscard]] llvm::hash_code
bvHashingHelper(llvm::ArrayRef<uintptr_t> Words) noexcept;
} // namespace detail

[[nodiscard]] inline llvm::hash_code
// NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
hash_value(const llvm::BitVector &BV) noexcept {
  if (BV.empty()) {
    return {};
  }
  return detail::bvHashingHelper(BV.getData());
}

[[nodiscard]] inline llvm::hash_code
// NOLINTNEXTLINE(readability-identifier-naming) -- needed for ADL
hash_value(const llvm::SmallBitVector &BV) noexcept {
  if (BV.empty()) {
    return {};
  }

  uintptr_t Store{};
  return detail::bvHashingHelper(BV.getData(Store));
}
} // namespace psr

#endif // PHASAR_UTILS_BITSETHASH_H
