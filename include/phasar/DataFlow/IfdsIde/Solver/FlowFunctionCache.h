#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWFUNCTIONCACHE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWFUNCTIONCACHE_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/GenericFlowFunction.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowFunctionCacheStats.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/PointerUtils.h"
#include "phasar/Utils/TableWrappers.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace psr {
namespace detail {
template <typename T, typename D, typename = void>
constexpr inline bool IsFlowFunction = false;

template <typename T, typename D>
constexpr inline bool
    IsFlowFunction<T, D,
                   std::void_t<decltype(std::declval<T>().computeTargets(
                       std::declval<D>()))>> = true;

template <typename T, typename D, typename = void>
constexpr inline bool IsFlowFunctionPtr = false;

template <typename T, typename D>
constexpr inline bool
    IsFlowFunctionPtr<T, D,
                      std::void_t<decltype(std::declval<T>()->computeTargets(
                          std::declval<D>()))>> = true;

template <typename D, typename FFTy> struct AutoAddZeroFF {
  FFTy FF;
  D ZeroValue;

  auto computeTargets(ByConstRef<D> Source) {
    auto Ret = FF.computeTargets(Source);
    if (Source == ZeroValue) {
      Ret.insert(ZeroValue);
    }
    return Ret;
  }

  [[nodiscard]] bool operator==(std::nullptr_t) const noexcept {
    if constexpr (std::is_same_v<FFTy, GenericFlowFunction<D>> ||
                  std::is_same_v<FFTy, GenericFlowFunctionView<D>>) {
      return FF == nullptr;
    } else {
      return std::is_same_v<std::decay_t<FFTy>, std::nullptr_t>;
    }
  }
  [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept {
    return !(*this == nullptr);
  }
};

template <typename D, typename FFTy>
AutoAddZeroFF(FFTy, D) -> AutoAddZeroFF<std::decay_t<D>, std::decay_t<FFTy>>;

template <typename NormalFFType, typename CtrFFType>
struct IntraFlowFunctionsMixin {
  DenseTable1d<uint64_t, NormalFFType> NormalFFCache{};
  /// NOTE: The Callees set is completely implied by CallSite/RetSite and does
  /// not need to be stored! Note2: Assume here that the callees set is
  /// invariant during the IFDS/IDE analysis
  DenseTable1d<uint64_t, CtrFFType> CtrFFCache{};

  void reserveIntraFFs(size_t NumInsts, size_t NumCalls) {
    NormalFFCache.reserve(NumInsts);
    CtrFFCache.reserve(NumCalls);
  }
};

template <typename IntraFFTy>
struct IntraFlowFunctionsMixin<IntraFFTy, IntraFFTy> {
  /// Whenever possible, use only one map for normal-flows and ctr-flows. The
  /// key-spaces are strictly disjoint and paying 75% MaxLoadFactor once less
  /// should be good for memory usage

  DenseTable1d<uint64_t, IntraFFTy> NormalFFCache{};
  DenseTable1d<uint64_t, IntraFFTy> &CtrFFCache = NormalFFCache;

  void reserveIntraFFs(size_t NumInsts, size_t /*NumCalls*/) {
    NormalFFCache.reserve(NumInsts);
  }
};

template <typename ProblemTy> struct FlowFunctionCacheBase {
  using domain_t = typename ProblemTy::ProblemAnalysisDomain;
  using d_t = typename domain_t::d_t;
  using n_t = typename domain_t::n_t;
  using f_t = typename domain_t::f_t;
  using normal_ff_t =
      std::decay_t<decltype(std::declval<ProblemTy>().getNormalFlowFunction(
          std::declval<n_t>(), std::declval<n_t>()))>;
  using call_ff_t =
      std::decay_t<decltype(std::declval<ProblemTy>().getCallFlowFunction(
          std::declval<n_t>(), std::declval<f_t>()))>;
  using ret_ff_t =
      std::decay_t<decltype(std::declval<ProblemTy>().getRetFlowFunction(
          std::declval<n_t>(), std::declval<f_t>(), std::declval<n_t>(),
          std::declval<n_t>()))>;
  using ctr_ff_t =
      std::decay_t<decltype(std::declval<ProblemTy>().getCallToRetFlowFunction(
          std::declval<n_t>(), std::declval<n_t>(),
          std::declval<llvm::ArrayRef<f_t>>()))>;
  using summary_ff_t =
      std::decay_t<decltype(std::declval<ProblemTy>().getSummaryFlowFunction(
          std::declval<n_t>(), std::declval<f_t>()))>;

  static_assert(detail::IsFlowFunction<normal_ff_t, d_t> ||
                detail::IsFlowFunctionPtr<normal_ff_t, d_t>);
  static_assert(detail::IsFlowFunction<call_ff_t, d_t> ||
                detail::IsFlowFunctionPtr<call_ff_t, d_t>);
  static_assert(detail::IsFlowFunction<ret_ff_t, d_t> ||
                detail::IsFlowFunctionPtr<ret_ff_t, d_t>);
  static_assert(detail::IsFlowFunction<ctr_ff_t, d_t> ||
                detail::IsFlowFunctionPtr<ctr_ff_t, d_t>);
  static_assert(detail::IsFlowFunction<summary_ff_t, d_t> ||
                detail::IsFlowFunctionPtr<summary_ff_t, d_t> ||
                std::is_same_v<summary_ff_t, std::nullptr_t>);

  template <typename T>
  static constexpr bool needs_cache_v =
      std::is_same_v<std::shared_ptr<FlowFunction<d_t>>, T> ||
      std::is_same_v<GenericFlowFunction<d_t>, T> ||
      (detail::IsFlowFunctionPtr<T, d_t> && !std::is_pointer_v<T>);
};

} // namespace detail

template <typename ProblemTy, bool AutoAddZero>
class FlowFunctionCache
    : detail::FlowFunctionCacheBase<ProblemTy>,
      detail::IntraFlowFunctionsMixin<
          typename detail::FlowFunctionCacheBase<ProblemTy>::normal_ff_t,
          typename detail::FlowFunctionCacheBase<ProblemTy>::ctr_ff_t> {
  using base_t = detail::FlowFunctionCacheBase<ProblemTy>;

  using typename base_t::call_ff_t;
  using typename base_t::ctr_ff_t;
  using typename base_t::d_t;
  using typename base_t::domain_t;
  using typename base_t::f_t;
  using typename base_t::n_t;
  using typename base_t::normal_ff_t;
  using typename base_t::ret_ff_t;
  using typename base_t::summary_ff_t;

  using detail::IntraFlowFunctionsMixin<normal_ff_t, ctr_ff_t>::NormalFFCache;
  using detail::IntraFlowFunctionsMixin<normal_ff_t, ctr_ff_t>::CtrFFCache;

  template <typename FFTy>
  static constexpr bool needs_cache_v = base_t::template needs_cache_v<FFTy>;

  template <typename FFTy>
  static auto wrapFlowFunction(ProblemTy &Problem, FFTy &&FF) {
    using ff_t = std::decay_t<FFTy>;

    if constexpr (AutoAddZero) {
      if constexpr (detail::IsFlowFunctionPtr<ff_t, d_t>) {
        if constexpr (needs_cache_v<ff_t>) {
          return detail::AutoAddZeroFF{
              GenericFlowFunctionView(getPointerFrom(FF)),
              Problem.getZeroValue()};
        } else {
          return detail::AutoAddZeroFF{
              GenericFlowFunction<d_t>(std::forward<FFTy>(FF)),
              Problem.getZeroValue()};
        }
      } else {
        return detail::AutoAddZeroFF{std::forward<FFTy>(FF),
                                     Problem.getZeroValue()};
      }
    } else {
      if constexpr (detail::IsFlowFunctionPtr<ff_t, d_t>) {
        if constexpr (needs_cache_v<ff_t>) {
          return GenericFlowFunctionView(getPointerFrom(FF));
        } else {
          return GenericFlowFunction<d_t>(std::forward<FFTy>(FF));
        }
      } else {
        return std::forward<FFTy>(FF);
      }
    }
  }

  template <typename FFTy, typename CacheTy, typename FFFactory>
  decltype(auto) getFlowFunction(CacheTy &Cache, uint64_t Id,
                                 ProblemTy &Problem, FFFactory Fact) {
    if constexpr (needs_cache_v<FFTy>) {
      auto &Ret = Cache.getOrCreate(Id);
      if (!Ret) {
        Ret = std::invoke(Fact, Problem);
      }

      return wrapFlowFunction(Problem, Ret);
    } else {
      return wrapFlowFunction(Problem, std::invoke(Fact, Problem));
    }
  }

public:
  FlowFunctionCache() noexcept = default;

  decltype(auto) getNormalFlowFunction(ProblemTy &Problem, ByConstRef<n_t> Curr,
                                       ByConstRef<n_t> Succ,
                                       uint64_t CurrSuccId) {
    return getFlowFunction<normal_ff_t>(
        NormalFFCache, CurrSuccId, Problem, [Curr, Succ](ProblemTy &Problem) {
          return Problem.getNormalFlowFunction(Curr, Succ);
        });
  }

  decltype(auto) getCallToRetFlowFunction(ProblemTy &Problem,
                                          ByConstRef<n_t> CallSite,
                                          ByConstRef<n_t> RetSite,
                                          llvm::ArrayRef<f_t> Callees,
                                          uint64_t CallRetId) {
    return getFlowFunction<ctr_ff_t>(
        CtrFFCache, CallRetId, Problem,
        [CallSite, RetSite, Callees](ProblemTy &Problem) {
          return Problem.getCallToRetFlowFunction(CallSite, RetSite, Callees);
        });
  }

  decltype(auto) getCallFlowFunction(ProblemTy &Problem,
                                     ByConstRef<n_t> CallSite,
                                     ByConstRef<f_t> Callee,
                                     uint64_t CSCalleeId) {
    return getFlowFunction<call_ff_t>(CallFFCache, CSCalleeId, Problem,
                                      [CallSite, Callee](ProblemTy &Problem) {
                                        return Problem.getCallFlowFunction(
                                            CallSite, Callee);
                                      });
  }

  /// CAUTION: Keep the ID definitions in mind!
  decltype(auto) getRetFlowFunction(ProblemTy &Problem,
                                    ByConstRef<n_t> CallSite,
                                    ByConstRef<f_t> Callee,
                                    ByConstRef<n_t> ExitInst,
                                    ByConstRef<n_t> RetSite, uint64_t CSExitId,
                                    uint64_t CalleeRSId) {
    if constexpr (needs_cache_v<ret_ff_t>) {
      if constexpr (std::is_base_of_v<
                        llvm::Instruction,
                        std::remove_cv_t<std::remove_pointer_t<n_t>>>) {
        if (llvm::isa<llvm::CallInst>(CallSite)) {
          auto &Ret = SimpleRetFFCache.getOrCreate(CSExitId);
          if (!Ret) {
            Ret =
                Problem.getRetFlowFunction(CallSite, Callee, ExitInst, RetSite);
          }
          return wrapFlowFunction(Problem, Ret);
        }
      }

      auto &Ret = RetFFCache.getOrCreate(std::make_pair(CSExitId, CalleeRSId));
      if (!Ret) {
        Ret = Problem.getRetFlowFunction(CallSite, Callee, ExitInst, RetSite);
      }
      return wrapFlowFunction(Problem, Ret);
    } else {
      return wrapFlowFunction(
          Problem,
          Problem.getRetFlowFunction(CallSite, Callee, ExitInst, RetSite));
    }
  }

  decltype(auto) getSummaryFlowFunction(ProblemTy &Problem,
                                        ByConstRef<n_t> CallSite,
                                        ByConstRef<f_t> Callee,
                                        uint64_t /*CSCalleeId*/) {
    if constexpr (needs_cache_v<summary_ff_t>) {
      return GenericFlowFunction<d_t>(
          Problem.getSummaryFlowFunction(CallSite, Callee));
    } else {
      return Problem.getSummaryFlowFunction(CallSite, Callee);
    }

    /// XXX: The old FECache doesn't cache summary FFs, so refrain from doing
    /// this here as well. Enable it, once the user-problems reliably return
    /// std::nullptr_t such that caching can be disabled automatically
  }

  void clear() noexcept {
    NormalFFCache.clear();
    CallFFCache.clear();
    RetFFCache.clear();
    SimpleRetFFCache.clear();
    CtrFFCache.clear();
    SummaryFFCache.clear();
  }

  void reserve(size_t NumInsts, size_t NumCalls, size_t NumFuns) {
    detail::IntraFlowFunctionsMixin<normal_ff_t, ctr_ff_t>::reserveIntraFFs(
        NumInsts, NumCalls);
    CallFFCache.reserve(NumCalls);
    RetFFCache.reserve(NumFuns);
    SimpleRetFFCache.reserve(NumFuns);
  }

  void dumpStats(llvm::raw_ostream &OS) const { OS << getStats(); }

  [[nodiscard]] FlowFunctionCacheStats getStats() const noexcept {
    /// TODO (#734): With C++20 use designated aggregate initializers here!
    return FlowFunctionCacheStats{
        NormalFFCache.size(),    CallFFCache.size(), RetFFCache.size(),
        SimpleRetFFCache.size(), CtrFFCache.size(),  SummaryFFCache.size(),
    };
  }

private:
  DenseTable1d<uint64_t, call_ff_t> CallFFCache;
  /// NOTE: Unfortunately, RetSite cannot imply CallSite (c.f. invoke
  /// instructions). Although ExitInst implies CalleeFun, it does not help to
  /// store a pair<uint64_t, uint32_t> --> padding
  ///
  /// Keys are (CS, ExitInst, Callee, RetSite)
  DenseTable1d<std::pair<uint64_t, uint64_t>, ctr_ff_t> RetFFCache;
  /// Here, we restrict ourselves to simple calls:
  /// Keys are (CS, ExitInst)
  DenseTable1d<uint64_t, ctr_ff_t> SimpleRetFFCache;

  DenseTable1d<uint64_t, summary_ff_t> SummaryFFCache;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_FLOWFUNCTIONCACHE_H
