/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_UTILS_MAPUTILS_H
#define PHASAR_UTILS_MAPUTILS_H

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/Macros.h"

#include "llvm/ADT/STLForwardCompat.h"

#include <type_traits>
#include <utility>

namespace psr {

template <typename MapT, typename KeyT,
          typename = std::enable_if_t<std::is_lvalue_reference_v<MapT>>>
static auto getOrDefault(MapT &&Map, KeyT &&Key) -> ByConstRef<
    llvm::remove_cvref_t<decltype(Map.find(PSR_FWD(Key))->second)>> {
  auto It = Map.find(PSR_FWD(Key));
  if (It == Map.end()) {
    return default_value();
  }

  return It->second;
}

template <
    typename MapT, typename KeyT,
    typename = std::enable_if_t<std::is_lvalue_reference_v<MapT>>,
    std::enable_if_t<
        !psr::CanEfficientlyPassByValue<llvm::remove_cvref_t<KeyT>>, int> = 0>
static auto getOrNull(MapT &&Map, KeyT &&Key)
    -> decltype(&Map.find(PSR_FWD(Key))->second) {
  auto It = Map.find(PSR_FWD(Key));
  decltype(&It->second) Ret = nullptr;
  if (It != Map.end()) {
    Ret = &It->second;
  }

  return Ret;
}

template <
    typename MapT, typename KeyT,
    typename = std::enable_if_t<std::is_lvalue_reference_v<MapT>>,
    std::enable_if_t<psr::CanEfficientlyPassByValue<llvm::remove_cvref_t<KeyT>>,
                     int> = 0>
static auto getOrNull(MapT &&Map, KeyT Key)
    -> decltype(&Map.find(Key)->second) {
  auto It = Map.find(Key);
  decltype(&It->second) Ret = nullptr;
  if (It != Map.end()) {
    Ret = &It->second;
  }

  return Ret;
}
} // namespace psr

#endif // PHASAR_UTILS_MAPUTILS_H
