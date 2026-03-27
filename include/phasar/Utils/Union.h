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

#include "phasar/Utils/Macros.h"

#include <utility>

namespace psr {
template <typename SetLT, typename SetRT>
[[nodiscard]] std::decay_t<SetLT> setUnion(SetLT &&First, SetRT &&Second,
                                           bool *ChangedPtr = nullptr) {
  const bool FirstSmaller = First.size() < Second.size();
  auto &Smaller = FirstSmaller ? First : Second;

  bool ChangedBuf = false;
  bool &Changed = ChangedPtr ? *ChangedPtr : ChangedBuf;

  auto Ret = [&] {
    if (FirstSmaller) {
      return std::decay_t<SetLT>(PSR_FWD(Second));
    }
    return PSR_FWD(First);
  }();
  for (auto &&Elem : Smaller) {
    Changed |= Ret.insert(Elem).second;
  }
  return Ret;
}

} // namespace psr

#endif // PHASAR_UTILS_UNOIN_H
