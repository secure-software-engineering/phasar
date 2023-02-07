/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONSINGLETONCACHE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONSINGLETONCACHE_H

#include "phasar/PhasarLLVM/Utils/ByRef.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/Support/ErrorHandling.h"

#include <type_traits>

namespace psr {

template <typename EdgeFunctionTy> class EdgeFunctionSingletonCache {
public:
  virtual ~EdgeFunctionSingletonCache() = default;

  [[nodiscard]] virtual const void *
  lookup(ByConstRef<EdgeFunctionTy> EF) const noexcept = 0;

  virtual void insert(const EdgeFunctionTy *EF, const void *Mem) = 0;
  virtual void erase(ByConstRef<EdgeFunctionTy> EF) noexcept = 0;
};

template <typename EdgeFunctionTy>
class DefaultEdgeFunctionSingletonCache
    : public EdgeFunctionSingletonCache<EdgeFunctionTy> {
public:
  DefaultEdgeFunctionSingletonCache() noexcept = default;

  DefaultEdgeFunctionSingletonCache(const DefaultEdgeFunctionSingletonCache &) =
      delete;
  DefaultEdgeFunctionSingletonCache &
  operator=(const DefaultEdgeFunctionSingletonCache &) = delete;

  DefaultEdgeFunctionSingletonCache(
      DefaultEdgeFunctionSingletonCache &&) noexcept = default;
  DefaultEdgeFunctionSingletonCache &
  operator=(DefaultEdgeFunctionSingletonCache &&) noexcept = delete;
  ~DefaultEdgeFunctionSingletonCache() override = default;

  [[nodiscard]] const void *
  lookup(ByConstRef<EdgeFunctionTy> EF) const noexcept override {
    return Cache.lookup(&EF);
  }

  void insert(const EdgeFunctionTy *EF, const void *Mem) override {
    auto [It, Inserted] = Cache.try_emplace(EF, Mem);
    assert(Inserted);
  }

  void erase(ByConstRef<EdgeFunctionTy> EF) noexcept override {
    Cache.erase(&EF);
  }

private:
  struct DSI : public llvm::DenseMapInfo<const EdgeFunctionTy *> {
    static bool isEqual(const EdgeFunctionTy *LHS,
                        const EdgeFunctionTy *RHS) noexcept {
      if (LHS == RHS) {
        return true;
      }
      auto Empty = llvm::DenseMapInfo<const EdgeFunctionTy *>::getEmptyKey();
      auto Tombstone =
          llvm::DenseMapInfo<const EdgeFunctionTy *>::getTombstoneKey();
      if (LHS == Empty || LHS == Tombstone || RHS == Empty ||
          RHS == Tombstone) {
        return false;
      }

      return *LHS == *RHS;
    }

    static auto getHashValue(const EdgeFunctionTy *EF) noexcept {
      assert(EF != llvm::DenseMapInfo<const EdgeFunctionTy *>::getEmptyKey());
      assert(EF !=
             llvm::DenseMapInfo<const EdgeFunctionTy *>::getTombstoneKey());

      return hash_value(*EF);
    }
  };

  llvm::DenseMap<const EdgeFunctionTy *, const void *, DSI> Cache;
};

template <typename EdgeFunctionTy> struct CachedEdgeFunction {
  using EdgeFunctionType = EdgeFunctionTy;

  EdgeFunctionTy EF{};
  EdgeFunctionSingletonCache<EdgeFunctionTy> *Cache{};
};

template <typename EdgeFunctionTy>
CachedEdgeFunction(EdgeFunctionTy, EdgeFunctionSingletonCache<EdgeFunctionTy> *)
    -> CachedEdgeFunction<EdgeFunctionTy>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONSINGLETONCACHE_H
