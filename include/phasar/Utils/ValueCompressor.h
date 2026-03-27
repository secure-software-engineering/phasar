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
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/PointerLikeTypeTraits.h"

#include <optional>
#include <type_traits>

/// The id-type for values inserted into the ValueCompressor.
PHASAR_STRONG_TYPEDEF(psr, uint32_t, ValueId);

namespace psr {

namespace detail {
template <typename T>
static constexpr bool IsPointerWithAtleastOneFreeLowBit = false;
template <typename T>
  requires requires() { llvm::PointerLikeTypeTraits<T>::NumLowBitsAvailable; }
static constexpr bool IsPointerWithAtleastOneFreeLowBit<T> =
    llvm::PointerLikeTypeTraits<T>::NumLowBitsAvailable > 0;
} // namespace detail

/// A utility-class that assigns sequential integer-like ids (psr::ValueId) to
/// every inserted value.
///
/// In contrast to psr::Compressor, the ValueCompressor can optionally
/// assign the same id to multiple values, and it can even produce ids without
/// corresponding values.
///
/// \tparam T The type of inserted values. Expected to be mostly small and
/// trivial.
template <typename T> class ValueCompressor {
public:
  using value_type = T;
  using id_type = ValueId;
  using var_aliases_type = std::conditional_t<
      detail::IsPointerWithAtleastOneFreeLowBit<T>, llvm::TinyPtrVector<T>,
      // Note: The inline-buffer of SmallVector is pointer-aligned, so it would
      // be wasteful to only allow one, e.g., int16
      llvm::SmallVector<T, std::max(size_t(1), sizeof(void *) / sizeof(T))>>;

  [[nodiscard]] constexpr const auto &id2vars() const noexcept
      [[clang::lifetimebound]] {
    return Id2Vars;
  }
  [[nodiscard]] constexpr const auto &id2vars(ValueId Id) const noexcept
      [[clang::lifetimebound]] {
    return Id2Vars[Id];
  }
  [[nodiscard]] constexpr const auto &var2id() const noexcept
      [[clang::lifetimebound]] {
    return Var2Id;
  }

  /// Inserts \p Var and assigns it a fresh \c ValueId, or returns the existing
  /// id if \p Var was already present.
  /// \returns {id, true} on first insertion; {id, false} if already present.
  std::pair<ValueId, bool> insert(ByConstRef<T> Var) {
    auto [It, Inserted] = Var2Id.try_emplace(Var, ValueId(Id2Vars.size()));
    if (Inserted) {
      Id2Vars.emplace_back().push_back(Var);
    }
    return {It->second, Inserted};
  }

  /// Registers \p Var as an additional name for the existing \p Id (aliasing).
  /// \p Id must already exist.  If \p Var already maps to some id, this is a
  /// no-op and returns \c false; otherwise the mapping is added and \c true is
  /// returned.
  bool addAlias(ByConstRef<T> Var, ValueId Id) {
    assert(Id2Vars.inbounds(Id) &&
           "Can only add an alias to an already existing Id!");
    if (Var2Id.try_emplace(Var, Id).second) {
      Id2Vars[Id].push_back(Var);
      return true;
    }
    return false;
  }

  /// Allocates a fresh \c ValueId with no associated variable (a placeholder
  /// or sentinel slot).
  [[nodiscard]] ValueId addDummy() {
    auto Ret = ValueId(Id2Vars.size());
    Id2Vars.emplace_back();
    return Ret;
  }

  [[nodiscard]] std::optional<ValueId> getOrNull(ByConstRef<T> Var) const {
    auto It = Var2Id.find(Var);
    if (It == Var2Id.end()) {
      return std::nullopt;
    }
    return It->second;
  }

  void reserve(size_t Capa) {
    Var2Id.reserve(Capa);
    Id2Vars.reserve(Capa);
  }

  [[nodiscard]] size_t size() const noexcept { return Id2Vars.size(); }
  [[nodiscard]] bool empty() const noexcept { return Id2Vars.empty(); }

private:
  llvm::DenseMap<T, ValueId> Var2Id;
  TypedVector<ValueId, var_aliases_type> Id2Vars;
};

} // namespace psr
