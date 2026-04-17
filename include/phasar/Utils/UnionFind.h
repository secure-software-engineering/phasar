#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/IntEqClasses.h"

namespace psr {

template <SmallIdType IdT = uint32_t, SmallIdType MappedIdT = IdT>
class CompressedUnionFind;

/// Mutable union-find (disjoint-set) data structure over integer-like ids.
///
/// Supports \c join() (union by rank with path compression, via
/// \c llvm::IntEqClasses) and \c find() (representative look-up). When all
/// unions have been performed, call \c compress() to obtain a
/// \c CompressedUnionFind that assigns dense sequential class indices.
///
/// \tparam IdT  Element type.  Must satisfy \c SmallIdType (fits in
///              \c uint32_t).
template <SmallIdType IdT = uint32_t> class UnionFind {
public:
  UnionFind() noexcept = default;
  explicit UnionFind(size_t InitSz) : Equiv(InitSz) {}

  IdT join(IdT L, IdT R) { return IdT(Equiv.join(unsigned(L), unsigned(R))); }

  [[nodiscard]] IdT find(IdT Val) const {
    return IdT(Equiv.findLeader(unsigned(Val)));
  }

  void grow(size_t NewSz) { Equiv.grow(NewSz); }

  template <SmallIdType MappedIdT = IdT>
  [[nodiscard]] CompressedUnionFind<IdT, MappedIdT> compress() &&;

private:
  llvm::IntEqClasses Equiv;
};

/// Read-only view of a compressed union-find: maps each original element to a
/// dense sequential class index in the range [0, numClasses()).
///
/// Obtained by calling \c UnionFind::compress<MappedIdT>(); not directly
/// constructible.
///
/// \tparam IdT       Original element type.
/// \tparam MappedIdT Type for the dense class indices.
template <SmallIdType IdT, SmallIdType MappedIdT> class CompressedUnionFind {
public:
  [[nodiscard]] size_t numClasses() const noexcept {
    return Equiv.getNumClasses();
  }

  [[nodiscard]] MappedIdT operator[](IdT Id) const {
    return MappedIdT(Equiv[unsigned(Id)]);
  }

private:
  friend UnionFind<IdT>;
  CompressedUnionFind(llvm::IntEqClasses &&Equiv) : Equiv(std::move(Equiv)) {}

  llvm::IntEqClasses Equiv;
};

template <SmallIdType IdT>
template <SmallIdType MappedIdT>
inline CompressedUnionFind<IdT, MappedIdT> UnionFind<IdT>::compress() && {
  Equiv.compress();
  return {std::move(Equiv)};
}
} // namespace psr
