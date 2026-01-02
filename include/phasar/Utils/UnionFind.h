#pragma once

#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/IntEqClasses.h"

namespace psr {

template <SmallIdType IdT = uint32_t, SmallIdType MappedIdT = IdT>
class CompressedUnionFind;

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

template <SmallIdType IdT, SmallIdType MappedIdT> class CompressedUnionFind {
public:
  [[nodiscard]] size_t size() const noexcept { return Equiv.getNumClasses(); }

  [[nodiscard]] bool inbounds(IdT Id) const noexcept {
    return size_t(Id) < size();
  }

  [[nodiscard]] MappedIdT operator[](IdT Id) const {
    assert(inbounds(Id));
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
