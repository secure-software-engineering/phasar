#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_EDGEFUNCTIONCACHE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_EDGEFUNCTIONCACHE_H

/******************************************************************************
 * Copyright (c) 2022 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/Solver/EdgeFunctionCacheStats.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/EquivalenceClassMap.h"
#include "phasar/Utils/StableVector.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>

namespace psr {

template <typename ProblemTy>
class EdgeFunctionCache : private EdgeFunctionCacheStats {
  using domain_t = typename ProblemTy::ProblemAnalysisDomain;
  using d_t = typename domain_t::d_t;
  using n_t = typename domain_t::n_t;
  using f_t = typename domain_t::f_t;
  using l_t = typename domain_t::l_t;

public:
  EdgeFunctionCache() noexcept = default;

  using EdgeFunctionPtrType = EdgeFunction<l_t>;

  EdgeFunctionPtrType
  getNormalEdgeFunction(ProblemTy &Problem, ByConstRef<n_t> Curr,
                        ByConstRef<d_t> CurrNode, ByConstRef<n_t> Succ,
                        ByConstRef<d_t> SuccNode, uint64_t CurrSuccId,
                        uint64_t CurrNodeSuccNodeId) {
    auto &EqMap = NormalAndCtrEFCache[CurrSuccId];
    if (!EqMap) {
      EqMap = &NormalMapOwner.emplace_back();
    }
    return EqMap->getOrInsertLazy(CurrNodeSuccNodeId, [&] {
      ++NormalAndCtrEFCacheCumulSize;
      return Problem.getNormalEdgeFunction(Curr, CurrNode, Succ, SuccNode);
    });
  }

  EdgeFunctionPtrType
  getCallToRetEdgeFunction(ProblemTy &Problem, ByConstRef<n_t> CallSite,
                           ByConstRef<d_t> CallNode, ByConstRef<n_t> RetSite,
                           ByConstRef<d_t> RetSiteNode,
                           llvm::ArrayRef<f_t> Callees, uint64_t CallRetId,
                           uint64_t CallNodeRetNodeId) {
    auto &EqMap = NormalAndCtrEFCache[CallRetId];
    if (!EqMap) {
      EqMap = &NormalMapOwner.emplace_back();
    }
    return EqMap->getOrInsertLazy(CallNodeRetNodeId, [&] {
      ++NormalAndCtrEFCacheCumulSize;
      return Problem.getCallToRetEdgeFunction(CallSite, CallNode, RetSite,
                                              RetSiteNode, Callees);
    });
  }

  EdgeFunctionPtrType
  getCallEdgeFunction(ProblemTy &Problem, ByConstRef<n_t> CallSite,
                      ByConstRef<d_t> SrcNode, ByConstRef<f_t> Callee,
                      ByConstRef<d_t> DestNode, uint64_t CSCalleeId,
                      uint64_t SrcDestNodeId) {
    auto &EqMap = CallEFCache[CSCalleeId];
    if (!EqMap) {
      EqMap = &NormalMapOwner.emplace_back();
    }
    return EqMap->getOrInsertLazy(SrcDestNodeId, [&] {
      ++CallEFCacheCumulSize;
      return Problem.getCallEdgeFunction(CallSite, SrcNode, Callee, DestNode);
    });
  }

  EdgeFunctionPtrType
  getReturnEdgeFunction(ProblemTy &Problem, ByConstRef<n_t> CallSite,
                        ByConstRef<f_t> Callee, ByConstRef<n_t> ExitInst,
                        ByConstRef<d_t> ExitNode, ByConstRef<n_t> RetSite,
                        ByConstRef<d_t> RetSiteNode, uint32_t ExitId,
                        uint64_t CSRSId, uint64_t ExitRSNodeId) {
    auto &EqMap = RetEFCache[ExitId];
    if (!EqMap) {
      EqMap = &RetMapOwner.emplace_back();
    }
    return EqMap->getOrInsertLazy(std::make_pair(CSRSId, ExitRSNodeId), [&] {
      ++RetEFCacheCumulSize;
      return Problem.getReturnEdgeFunction(CallSite, Callee, ExitInst, ExitNode,
                                           RetSite, RetSiteNode);
    });
  }

  EdgeFunctionPtrType
  getSummaryEdgeFunction(ProblemTy &Problem, ByConstRef<n_t> CallSite,
                         ByConstRef<d_t> CallNode, ByConstRef<n_t> RetSite,
                         ByConstRef<d_t> RetSiteNode, uint64_t CSRSId,
                         uint64_t CSNodeRSNodeId) {
    auto &EqMap = SummaryEFCache[CSRSId];
    if (!EqMap) {
      EqMap = &NormalMapOwner.emplace_back();
    }
    return EqMap->getOrInsertLazy(CSNodeRSNodeId, [&] {
      ++SummaryEFCacheCumulSize;
      return Problem.getSummaryEdgeFunction(CallSite, CallNode, RetSite,
                                            RetSiteNode);
    });
  }

  void clear() {
    NormalAndCtrEFCache.clear();
    CallEFCache.clear();
    RetEFCache.clear();
    SummaryEFCache.clear();
    NormalMapOwner.clear();
    RetMapOwner.clear();
  }

  void dumpStats(llvm::raw_ostream &OS) const { OS << getStats(); }

  [[nodiscard]] EdgeFunctionCacheStats getStats() const noexcept {
    size_t Sz = 0;
    size_t MaxInnerSz = 0;
    llvm::SmallVector<unsigned> Sizes;
    for (const auto &[Edge, NormalAndCtrEF] : NormalAndCtrEFCache) {
      Sz += NormalAndCtrEF->size();
      MaxInnerSz = std::max(MaxInnerSz, NormalAndCtrEF->size());
      Sizes.push_back(NormalAndCtrEF->size());
    }

    std::sort(Sizes.begin(), Sizes.end());

    EdgeFunctionCacheStats Ret = *this;
    Ret.NormalAndCtrEFCacheCumulSizeReduced = Sz;
    Ret.AvgEFPerEdge = double(Sz) / NormalAndCtrEFCache.size();
    Ret.MaxEFPerEdge = MaxInnerSz;
    Ret.MedianEFPerEdge = Sizes.empty() ? 0 : Sizes[Sizes.size() / 2];
    return Ret;
  }

private:
  StableVector<EquivalenceClassMapNG<uint64_t, EdgeFunctionPtrType>>
      NormalMapOwner{};
  StableVector<
      EquivalenceClassMapNG<std::pair<uint64_t, uint64_t>, EdgeFunctionPtrType>>
      RetMapOwner{};
  /// NOTE: For CTR, the Callees set is implied by the CallSite/RetSite
  llvm::DenseMap<uint64_t,
                 EquivalenceClassMapNG<uint64_t, EdgeFunctionPtrType> *>
      NormalAndCtrEFCache{};
  llvm::DenseMap<uint64_t,
                 EquivalenceClassMapNG<uint64_t, EdgeFunctionPtrType> *>
      CallEFCache{};
  llvm::DenseMap<uint32_t, EquivalenceClassMapNG<std::pair<uint64_t, uint64_t>,
                                                 EdgeFunctionPtrType> *>
      RetEFCache{};
  llvm::DenseMap<uint64_t,
                 EquivalenceClassMapNG<uint64_t, EdgeFunctionPtrType> *>
      SummaryEFCache{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_EDGEFUNCTIONCACHE_H
