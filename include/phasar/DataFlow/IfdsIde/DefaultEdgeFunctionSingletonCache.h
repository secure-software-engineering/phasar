/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_DEFAULTEDGEFUNCTIONSINGLETONCACHE_H
#define PHASAR_DATAFLOW_IFDSIDE_DEFAULTEDGEFUNCTIONSINGLETONCACHE_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionSingletonCache.h"

namespace psr {

/// Default implementation of EdgeFunctionSingletonCache.
///
/// For an edge function EdgeFunctionTy to be cached, it must be hashable, i.e.
/// it must implement the friend- or nonmember function llvm::hash_code
/// hash_value(const EdgeFunctionTy&).
///
/// This cache is *not* thread-safe.
template <typename EdgeFunctionTy, typename = void>
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
    assert(EF != nullptr);
    auto [It, Inserted] = Cache.try_emplace(EF, Mem);
    assert(Inserted);
  }

  void erase(ByConstRef<EdgeFunctionTy> EF) noexcept override {
    Cache.erase(&EF);
  }

  template <typename... ArgTys>
  [[nodiscard]] EdgeFunction<typename EdgeFunctionTy::l_t>
  createEdgeFunction(ArgTys &&...Args) {
    return CachedEdgeFunction<EdgeFunctionTy>{
        EdgeFunctionTy{std::forward<ArgTys>(Args)...}, this};
  }

private:
  struct DSI : public llvm::DenseMapInfo<const EdgeFunctionTy *> {
    static bool isEqual(const EdgeFunctionTy *LHS,
                        const EdgeFunctionTy *RHS) noexcept {
      if (LHS == RHS) {
        return true;
      }
      const auto *Empty =
          llvm::DenseMapInfo<const EdgeFunctionTy *>::getEmptyKey();
      const auto *Tombstone =
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

template <typename EdgeFunctionTy>
class DefaultEdgeFunctionSingletonCache<
    EdgeFunctionTy,
    std::enable_if_t<EdgeFunctionBase::IsSOOCandidate<EdgeFunctionTy>>> {
public:
  [[nodiscard]] const void *
  lookup(ByConstRef<EdgeFunctionTy> /*EF*/) const noexcept override {
    return nullptr;
  }
  void insert(const EdgeFunctionTy * /*EF*/, const void * /*Mem*/) override {
    assert(false && "We should never go here");
  }
  void erase(ByConstRef<EdgeFunctionTy> /*EF*/) noexcept override {
    assert(false && "We should never go here");
  }
  [[nodiscard]] EdgeFunction<typename EdgeFunctionTy::l_t>
  createEdgeFunction(EdgeFunctionTy EF) {
    return EF;
  }
};

} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_DEFAULTEDGEFUNCTIONSINGLETONCACHE_H
