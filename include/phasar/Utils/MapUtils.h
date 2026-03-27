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

namespace psr {

template <typename MapT, typename KeyT>
  requires std::is_lvalue_reference_v<MapT>
inline auto getOrDefault(MapT &&Map, KeyT &&Key) -> ByConstRef<
    std::remove_cvref_t<decltype(Map.find(PSR_FWD(Key))->second)>> {
  auto It = Map.find(PSR_FWD(Key));
  if (It == Map.end()) {
    return default_value();
  }

  return It->second;
}

template <typename MapT, typename KeyT>
  requires(std::is_lvalue_reference_v<MapT> &&
           !psr::CanEfficientlyPassByValue<llvm::remove_cvref_t<KeyT>>)
inline auto getOrNull(MapT &&Map, KeyT &&Key)
    -> decltype(&Map.find(PSR_FWD(Key))->second) {
  auto It = Map.find(PSR_FWD(Key));
  decltype(&It->second) Ret = nullptr;
  if (It != Map.end()) {
    Ret = &It->second;
  }

  return Ret;
}

template <typename MapT, typename KeyT>
  requires(std::is_lvalue_reference_v<MapT> &&
           psr::CanEfficientlyPassByValue<llvm::remove_cvref_t<KeyT>>)
inline auto getOrNull(MapT &&Map, KeyT Key)
    -> decltype(&Map.find(Key)->second) {
  auto It = Map.find(PSR_FWD(Key));
  decltype(&It->second) Ret = nullptr;
  if (It != Map.end()) {
    Ret = &It->second;
  }

  return Ret;
}

template <typename MapT, typename KeyT, typename ValueT>
  requires CanEfficientlyPassByValue<
      typename std::remove_cvref_t<MapT>::mapped_type>
inline auto getOr(MapT &&Map, KeyT &&Key, ValueT &&FallbackVal)
    -> std::remove_cvref_t<decltype(Map.find(PSR_FWD(Key))->second)> {
  auto It = Map.find(PSR_FWD(Key));

  if (It == Map.end()) {
    return PSR_FWD(FallbackVal);
  }

  return It->second;
}

template <typename MapT, typename KeyT>
  requires(!CanEfficientlyPassByValue<
               typename std::remove_cvref_t<MapT>::mapped_type> &&
           std::is_lvalue_reference_v<MapT>)
inline auto
getOr(MapT &&Map, KeyT &&Key,
      const typename std::remove_cvref_t<MapT>::mapped_type &FallbackVal)
    -> decltype(Map.find(PSR_FWD(Key))->second) const & {
  auto It = Map.find(PSR_FWD(Key));

  if (It == Map.end()) {
    return FallbackVal;
  }

  return It->second;
}

template <typename MapT, typename KeyT>
  requires(!CanEfficientlyPassByValue<
              typename std::remove_cvref_t<MapT>::mapped_type>)
auto getOr(MapT &&Map, KeyT &&Key,
           const typename std::remove_cvref_t<MapT>::mapped_type &&FallbackVal)
    -> decltype(Map.find(PSR_FWD(Key))->second) const & = delete;
} // namespace psr

#endif // PHASAR_UTILS_MAPUTILS_H
