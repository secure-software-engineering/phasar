/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_UNOIN_H
#define PHASAR_UTILS_UNOIN_H

#include <utility>

namespace psr {
template <typename SetT>
[[nodiscard]] SetT setUnion(SetT First, SetT Second,
                            bool *ChangedPtr = nullptr) {
  bool FirstSmaller = First.size() < Second.size();
  auto &Smaller = FirstSmaller ? First : Second;

  bool ChangedBuf = false;
  bool &Changed = ChangedPtr ? *ChangedPtr : ChangedBuf;

  auto Ret = std::move(FirstSmaller ? Second : First);
  for (auto &&Elem : Smaller) {
    Changed |= Ret.insert(Elem).second;
  }
  return Ret;
}
} // namespace psr

#endif // PHASAR_UTILS_UNOIN_H
