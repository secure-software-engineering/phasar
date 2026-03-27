#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/StrongTypeDef.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"

#include <array>
#include <concepts>
#include <cstdint>

namespace psr {
/// A K-limited call-string context.  Stores the \p K most recent call-sites
/// that led to the current execution point (index 0 = most recent).  The
/// default \p K = 1 gives a 1-call-site context (CFA/k-CFA style).
template <typename N, unsigned K = 1> struct CallingContext {
  using n_t = N;

  /// The K most recent call-sites; index 0 is the most recent.
  std::array<n_t, K> Callers{};

  [[nodiscard]] friend auto hash_value(const CallingContext &CC) {
    return llvm::hash_combine_range(CC.Callers.begin(), CC.Callers.end());
  }

  [[nodiscard]] friend bool
  operator==(const CallingContext &L,
             const CallingContext &R) noexcept = default;

  [[nodiscard]] CallingContext withPrefix(n_t FirstCS) const noexcept {
    auto Ret = *this;
    auto Tmp = FirstCS;
    for (size_t I = 0; I != K; ++I) {
      auto NextTmp = Ret.Callers[I];
      Ret.Callers[I] = Tmp;
      Tmp = NextTmp;
    }

    return Ret;
  }
};
} // namespace psr

namespace llvm {
template <typename N, unsigned K>
struct DenseMapInfo<psr::CallingContext<N, K>> {
  using CallingContext = psr::CallingContext<N, K>;

  static CallingContext getEmptyKey() noexcept {
    CallingContext Ret{};
    Ret.Callers[0] = llvm::DenseMapInfo<N>::getEmptyKey();
    return Ret;
  }
  static CallingContext getTombstoneKey() noexcept {
    CallingContext Ret{};
    Ret.Callers[0] = llvm::DenseMapInfo<N>::getTombstoneKey();
    return Ret;
  }

  static auto getHashValue(psr::ByConstRef<CallingContext> CC) {
    return hash_value(CC);
  }

  static bool isEqual(psr::ByConstRef<CallingContext> L,
                      psr::ByConstRef<CallingContext> R) noexcept {
    return L == R;
  }
};
} // namespace llvm

PHASAR_STRONG_TYPEDEF(psr, uint32_t, CallingContextId, None = 0);

namespace psr {

/// Manages the set of calling contexts encountered for a given analysis and
/// maps them to dense sequential \c CallingContextId values.
///
/// Used by \c CallingContextSensUnionFindAA to enumerate all call-string
/// prefixes of length ≤ K for each function.  Contexts are interned in a
/// \c Compressor; \c CallingContextId::None (id 0) always represents the
/// "no calling context" (top-level / uncalled) entry.
///
/// \tparam N  Node type (call-site).
/// \tparam F  Function type.
/// \tparam K  Maximum call-string depth (must be in [1, 9]).
template <typename N, typename F, unsigned K = 1>
class CallingContextConstructor {
public:
  static_assert(K > 0);
  static_assert(K < 10, "Do you really want a sooo large context-k-limit? "
                        "Reconsider your choices!");

  using n_t = N;
  using f_t = F;

  CallingContextConstructor() {
    // Assign Id 0 === ContextId::None
    CC2Id.getOrInsert(CallingContext<n_t, K>{});
  }

  /// Enumerates all call-string prefixes of length ≤ K that lead to \p Fun
  /// and invokes \p CCVisitor(firstCallSite, contextId) for each one.
  /// If \p Fun has no callers (or K is exhausted), the "None" context is used.
  template <ProjectIRDB DBTy>
    requires(std::same_as<typename DBTy::f_t, f_t> &&
             std::same_as<typename DBTy::n_t, n_t>)
  void visitAllCallingContexts(
      ByConstRef<f_t> Fun, const CallGraph<n_t, f_t> &CG, const DBTy &IRDB,
      std::invocable<n_t, CallingContextId> auto CCVisitor) {
    CallingContext<n_t, K> Ctx{};
    visitAllCallingContextsImpl<0>(Fun, CG, IRDB, CCVisitor, Ctx);
  }

  /// Enumerates all calling contexts that include \p Call as their most recent
  /// call-site and invokes \p CCVisitor(contextId) for each one.
  template <ProjectIRDB DBTy>
    requires(std::same_as<typename DBTy::f_t, f_t> &&
             std::same_as<typename DBTy::n_t, n_t>)
  void visitContextsAtCall(ByConstRef<n_t> Call, const CallGraph<n_t, f_t> &CG,
                           const DBTy &IRDB,
                           std::invocable<CallingContextId> auto CCVisitor) {
    CallingContext<n_t, K> Ctx{};
    Ctx.Callers[0] = Call;
    visitAllCallingContextsImpl<1>(
        IRDB.getFunctionOf(Call), CG, IRDB,
        [CCVisitor{std::move(CCVisitor)}](ByConstRef<n_t>,
                                          CallingContextId Ctx) {
          std::invoke(CCVisitor, Ctx);
        },
        Ctx);
  }

  CallingContextId getOrInsert(ByConstRef<CallingContext<n_t, K>> CC) {
    return CC2Id.getOrInsert(CC);
  }

  [[nodiscard]] ByConstRef<CallingContext<n_t, K>>
  operator[](CallingContextId Ctx) {
    return CC2Id[Ctx];
  }

private:
  template <unsigned Idx, typename DBTy>
  void visitAllCallingContextsImpl(
      ByConstRef<f_t> Fun, const CallGraph<n_t, f_t> &CG, const DBTy &IRDB,
      std::invocable<n_t, CallingContextId> auto &&CCVisitor,
      CallingContext<n_t, K> &CurrCtx) {
    // TODO: Improve this algorithm

    if constexpr (Idx < K) {
      auto &&CallersOfCS = CG.getCallersOf(Fun);

      for (const auto &CS : CallersOfCS) {
        CurrCtx.Callers[Idx] = CS;
        visitAllCallingContextsImpl<(Idx + 1)>(IRDB.getFunctionOf(CS), CG, IRDB,
                                               CCVisitor, CurrCtx);
      }

      if (!CallersOfCS.empty()) {
        return;
      }
    }

    auto CtxId = CC2Id.getOrInsert(CurrCtx);
    std::invoke(CCVisitor, CurrCtx.Callers[0], CtxId);
  }

  Compressor<CallingContext<n_t, K>, CallingContextId> CC2Id{};
};
} // namespace psr
