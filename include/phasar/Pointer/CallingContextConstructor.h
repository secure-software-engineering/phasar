#pragma once

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/ControlFlow/ICFGBase.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Compressor.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"

#include <array>

namespace psr {
template <typename N, unsigned K = 1> struct CallingContext {
  using n_t = N;

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

namespace psr {
enum class CallingContextId : uint32_t { None = 0 };

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

  template <typename CFGTy>
  void visitAllCallingContexts(
      const llvm::Function *Fun, const CallGraph<n_t, f_t> &CG,
      const CFGBase<CFGTy> &CF,
      std::invocable<n_t, CallingContextId> auto CCVisitor) {
    CallingContext<n_t, K> Ctx{};
    visitAllCallingContextsImpl<0>(Fun, CG, CF, CCVisitor, Ctx);
  }

  template <typename CFGTy>
  void visitContextsAtCall(ByConstRef<n_t> Call, const CallGraph<n_t, f_t> &CG,
                           const CFGBase<CFGTy> &CF,
                           std::invocable<CallingContextId> auto CCVisitor) {
    CallingContext<n_t, K> Ctx{};
    Ctx.Callers[0] = Call;
    visitAllCallingContextsImpl<1>(
        CF.getFunctionOf(Call), CG, CF,
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
  template <typename CFGTy, unsigned Idx>
  void visitAllCallingContextsImpl(
      ByConstRef<f_t> Fun, const CallGraph<n_t, f_t> &CG,
      const CFGBase<CFGTy> &CF,
      std::invocable<n_t, CallingContextId> auto &&CCVisitor,
      CallingContext<n_t, K> &CurrCtx) {
    // TODO: Improve this algorithm

    if constexpr (Idx < K) {
      auto &&CallersOfCS = CG.getCallersOf(Fun);

      for (const auto &CS : CallersOfCS) {
        CurrCtx.Callers[Idx] = CS;
        visitAllCallingContextsImpl<(Idx + 1)>(CF.getFunctionOf(CS), CG, CF,
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
