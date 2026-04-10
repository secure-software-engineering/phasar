/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/
#pragma once

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/DenseMapInfo.h"

#include <cstdint>
#include <type_traits>

namespace psr {

namespace detail {
// Unfortunately, `enum class` cannot be templated, but we want type-safety for
// SCC-IDs...
struct SCCIdBase {
  uint32_t Value{};

  constexpr SCCIdBase() noexcept = default;

  explicit constexpr SCCIdBase(uint32_t Val) noexcept : Value(Val) {}

  explicit constexpr operator uint32_t() const noexcept { return Value; }
  template <typename T = size_t>
  explicit constexpr operator size_t() const noexcept
    requires(!std::is_same_v<uint32_t, T>)
  {
    return Value;
  }

  explicit constexpr operator ptrdiff_t() const noexcept {
    return ptrdiff_t(Value);
  }

  constexpr uint32_t operator+() const noexcept { return Value; }

  friend constexpr bool operator==(SCCIdBase L, SCCIdBase R) noexcept {
    return L.Value == R.Value;
  }
  friend constexpr bool operator!=(SCCIdBase L, SCCIdBase R) noexcept {
    return !(L == R);
  }
};
} // namespace detail

/// \brief The Id of a strongly-connected component in a graph.
///
/// \tparam GraphNodeId The vertex-type of the graph where this SCC was computed
/// for.
template <typename GraphNodeId> struct SCCId : detail::SCCIdBase {
  using detail::SCCIdBase::SCCIdBase;
};

static_assert(IdType<SCCId<uint32_t>>);

} // namespace psr

namespace llvm {
template <typename GraphNodeId> struct DenseMapInfo<psr::SCCId<GraphNodeId>> {
  using SCCId = psr::SCCId<GraphNodeId>;

  static constexpr SCCId getEmptyKey() noexcept { return SCCId(UINT32_MAX); }
  static constexpr SCCId getTombstoneKey() noexcept {
    return SCCId(UINT32_MAX - 1);
  }

  static auto getHashValue(SCCId SCC) noexcept {
    return llvm::hash_value(uint32_t(SCC));
  }
  static constexpr bool isEqual(SCCId SCC1, SCCId SCC2) noexcept {
    return SCC1 == SCC2;
  }
};
} // namespace llvm
