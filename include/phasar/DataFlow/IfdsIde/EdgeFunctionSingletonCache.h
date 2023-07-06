/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONSINGLETONCACHE_H
#define PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONSINGLETONCACHE_H

#include "phasar/Utils/ByRef.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/Support/ErrorHandling.h"

#include <type_traits>

namespace psr {

template <typename EdgeFunctionTy> class EdgeFunctionSingletonCache;

/// Wrapper over an edge function and a pointer to a compatible
/// EdgeFunctionSingletonCache.
///
/// When converting CachedEdgeFunction to EdgeFunction, signals to the
/// EdgeFunction-allocator that, if not small-object-optimized, this edge
/// function EF wishes to be allocated within the provided cache.
///
/// Cache may not be nullptr.
template <typename EdgeFunctionTy> struct CachedEdgeFunction {
  using EdgeFunctionType = EdgeFunctionTy;

  EdgeFunctionTy EF{};
  EdgeFunctionSingletonCache<EdgeFunctionTy> *Cache{};
};

/// Base-class for edge function caches. Compatible with the caching mechanism
/// built in EdgeFunction.
template <typename EdgeFunctionTy> class EdgeFunctionSingletonCache {
public:
  virtual ~EdgeFunctionSingletonCache() = default;

  /// Checks whether the edge function EF is cached in this cache. Returns the
  /// cached entry if found, else nullptr.
  [[nodiscard]] virtual const void *
  lookup(ByConstRef<EdgeFunctionTy> EF) const noexcept = 0;

  /// Inserts the cache-entry Mem for the edge function *EF into the cache.
  /// Typically, EF points into the buffer pointed to by Mem. Both pointers are
  /// being stored and should not be null.
  ///
  /// Precondition: *EF Must not be in the cache.
  virtual void insert(const EdgeFunctionTy *EF, const void *Mem) = 0;

  /// Erases the cache-entry associated with the edge function EF from the
  /// cache.
  virtual void erase(ByConstRef<EdgeFunctionTy> EF) noexcept = 0;

  template <typename... ArgTys>
  [[nodiscard]] auto createEdgeFunction(ArgTys &&...Args) {
    return CachedEdgeFunction<EdgeFunctionTy>{
        EdgeFunctionTy{std::forward<ArgTys>(Args)...}, this};
  }
};

template <typename EdgeFunctionTy>
CachedEdgeFunction(EdgeFunctionTy, EdgeFunctionSingletonCache<EdgeFunctionTy> *)
    -> CachedEdgeFunction<EdgeFunctionTy>;

} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONSINGLETONCACHE_H
