/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_MULTIEDGEFUNCTIONSINGLETONCACHE_H
#define PHASAR_DATAFLOW_IFDSIDE_MULTIEDGEFUNCTIONSINGLETONCACHE_H

#include "phasar/DataFlow/IfdsIde/DefaultEdgeFunctionSingletonCache.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include <optional>
#include <tuple>
#include <utility>

namespace psr {
template <typename... Ts> class MultiEdgeFunctionSingletonCache {
  static_assert(sizeof...(Ts) > 0);

  template <typename First, typename... Rest>
  static constexpr auto latticePicker() {
    return type_identity<typename First::l_t>{};
  };

  template <size_t I, typename U, typename Tup>
  static constexpr std::optional<size_t> tupleIndex() {
    if constexpr (I >= std::tuple_size_v<Tup>) {
      return std::nullopt;
    } else if constexpr (std::is_same_v<U, std::tuple_element_t<I, Tup>>) {
      return I;
    } else {
      return tupleIndex<I + 1, U, Tup>();
    }
  }

public:
  using l_t = typename decltype(latticePicker<Ts...>())::type;

  MultiEdgeFunctionSingletonCache() noexcept = default;
  MultiEdgeFunctionSingletonCache(const MultiEdgeFunctionSingletonCache &) =
      delete;
  MultiEdgeFunctionSingletonCache &
  operator=(const MultiEdgeFunctionSingletonCache &) = delete;

  MultiEdgeFunctionSingletonCache(MultiEdgeFunctionSingletonCache &&) noexcept =
      delete;
  MultiEdgeFunctionSingletonCache &
  operator=(MultiEdgeFunctionSingletonCache &&) noexcept = delete;
  ~MultiEdgeFunctionSingletonCache() = default;

  template <typename U>
  [[nodiscard]] const void *lookup(ByConstRef<U> EF) const noexcept {
    constexpr auto Idx = tupleIndex<0, U, std::tuple<Ts...>>();
    if (!Idx) {
      return nullptr;
    }

    auto &Cache = std::get<*Idx>(Caches);
    return Cache.lookup(EF);
  }

  template <typename U> void insert(const U *EF, const void *Mem) {
    constexpr auto Idx = tupleIndex<0, U, std::tuple<Ts...>>();
    static_assert(Idx.has_value());
    assert(EF != nullptr);

    auto &Cache = std::get<*Idx>(Caches);
    return Cache.insert(EF, Mem);
  }

  template <typename U> void erase(ByConstRef<U> EF) noexcept {
    constexpr auto Idx = tupleIndex<0, U, std::tuple<Ts...>>();
    static_assert(Idx.has_value());

    auto &Cache = std::get<*Idx>(Caches);
    return Cache.erase(EF);
  }

  template <typename U, typename... ArgTys>
  [[nodiscard]] EdgeFunction<l_t> createEdgeFunction(ArgTys &&...Args) {
    constexpr auto Idx = tupleIndex<0, U, std::tuple<Ts...>>();
    if (!Idx) {
      return U{std::forward<ArgTys>(Args)...};
    }

    auto &Cache = std::get<*Idx>(Caches);
    return Cache.createEdgeFunction(std::forward<ArgTys>(Args)...);
  }

private:
  std::tuple<DefaultEdgeFunctionSingletonCache<Ts>...> Caches;
};
} // namespace psr

#endif
