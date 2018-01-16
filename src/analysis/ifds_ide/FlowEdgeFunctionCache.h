/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef FLOWEDGEFUNCTIONCACHE_H_
#define FLOWEDGEFUNCTIONCACHE_H_

#include "EdgeFunction.h"
#include "FlowFunction.h"
#include "IDETabulationProblem.h"
#include "ZeroedFlowFunction.h"
#include <map>
#include <memory>
#include <tuple>
using namespace std;

/**
 * This class caches flow and edge functions to avoid their reconstruction.
 * When a flow or edge function must be applied to multiple times, a cached
 * version is used if existend, otherwise a new one is created and inserted
 * into the cache.
 */
template <typename N, typename D, typename M, typename V, typename I>
struct FlowEdgeFunctionCache {
  IDETabulationProblem<N, D, M, V, I> &problem;
  // Counters for the normal flow functions
  size_t normalFFConstruction = 0;
  size_t normalFFCacheHit = 0;
  // Counters for the call flow functions
  size_t callFFConstruction = 0;
  size_t callFFCacheHit = 0;
  // Counters for return flow functions
  size_t returnFFConstruction = 0;
  size_t returnFFCacheHit = 0;
  // Counters for the call to return flow functions
  size_t callToRetFFConstruction = 0;
  size_t callToRetFFCacheHit = 0;
  // Counters for the summary flow functions
  size_t summaryFFConstruction = 0;
  size_t summaryFFCacheHit = 0;
  // Counters for the normal edge functions
  size_t normalEFConstruction = 0;
  size_t normalEFCacheHit = 0;
  // Counters for the call edge functions
  size_t callEFConstruction = 0;
  size_t callEFCacheHit = 0;
  // Counters for the return edge functions
  size_t returnEFConstruction = 0;
  size_t returnEFCacheHit = 0;
  // Counters for the call to return edge functions
  size_t callToRetEFConstruction = 0;
  size_t callToRetEFCacheHit = 0;
  // Counters for the summary edge functions
  size_t summaryEFConstruction = 0;
  size_t summaryEFCacheHit = 0;
  // Auto add zero
  bool autoAddZero;
  D zeroValue;
  // Caches for the flow functions
  map<tuple<N, N>, shared_ptr<FlowFunction<D>>> NormalFlowFunctionCache;
  map<tuple<N, M>, shared_ptr<FlowFunction<D>>> CallFlowFunctionCache;
  map<tuple<N, M, N, N>, shared_ptr<FlowFunction<D>>> ReturnFlowFunctionCache;
  map<tuple<N, N>, shared_ptr<FlowFunction<D>>> CallToRetFlowFunctionCache;
  map<tuple<N, M>, shared_ptr<FlowFunction<D>>> SummaryFlowFunctionCache;
  // Caches for the edge functions
  map<tuple<N, D, N, D>, shared_ptr<EdgeFunction<V>>> NormalEdgeFunctionCache;
  map<tuple<N, D, M, D>, shared_ptr<EdgeFunction<V>>> CallEdgeFunctionCache;
  map<tuple<N, M, N, D, N, D>, shared_ptr<EdgeFunction<V>>>
      ReturnEdgeFunctionCache;
  map<tuple<N, D, N, D>, shared_ptr<EdgeFunction<V>>>
      CallToRetEdgeFunctionCache;
  map<tuple<N, D, N, D>, shared_ptr<EdgeFunction<V>>> SummaryEdgeFunctionCache;

  // Ctor allows access to the IDEProblem in order to get access to flow and
  // edge function factory functions.
  FlowEdgeFunctionCache(IDETabulationProblem<N, D, M, V, I> &p)
      : problem(p), autoAddZero(p.solver_config.autoAddZero),
        zeroValue(p.zeroValue()) {}

  shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) {
    auto key = tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      ++normalFFCacheHit;
      return NormalFlowFunctionCache.at(key);
    } else {
      ++normalFFConstruction;
      auto ff = (autoAddZero)
                    ? make_shared<ZeroedFlowFunction<D>>(
                          problem.getNormalFlowFunction(curr, succ), zeroValue)
                    : problem.getNormalFlowFunction(curr, succ);
      NormalFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt, M destMthd) {
    auto key = tie(callStmt, destMthd);
    if (CallFlowFunctionCache.count(key)) {
      ++callFFCacheHit;
      return CallFlowFunctionCache.at(key);
    } else {
      ++callFFConstruction;
      auto ff =
          (autoAddZero)
              ? make_shared<ZeroedFlowFunction<D>>(
                    problem.getCallFlowFunction(callStmt, destMthd), zeroValue)
              : problem.getCallFlowFunction(callStmt, destMthd);
      CallFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  shared_ptr<FlowFunction<D>> getRetFlowFunction(N callSite, M calleeMthd,
                                                 N exitStmt, N retSite) {
    auto key = tie(callSite, calleeMthd, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      ++returnFFCacheHit;
      return ReturnFlowFunctionCache.at(key);
    } else {
      ++returnFFConstruction;
      auto ff = (autoAddZero)
                    ? make_shared<ZeroedFlowFunction<D>>(
                          problem.getRetFlowFunction(callSite, calleeMthd,
                                                     exitStmt, retSite),
                          zeroValue)
                    : problem.getRetFlowFunction(callSite, calleeMthd, exitStmt,
                                                 retSite);
      ReturnFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  shared_ptr<FlowFunction<D>> getCallToRetFlowFunction(N callSite, N retSite) {
    auto key = tie(callSite, retSite);
    if (CallToRetFlowFunctionCache.count(key)) {
      ++callToRetFFCacheHit;
      return CallToRetFlowFunctionCache.at(key);
    } else {
      ++callToRetFFConstruction;
      auto ff = (autoAddZero)
                    ? make_shared<ZeroedFlowFunction<D>>(
                          problem.getCallToRetFlowFunction(callSite, retSite),
                          zeroValue)
                    : problem.getCallToRetFlowFunction(callSite, retSite);
      CallToRetFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N callStmt, M destMthd) {
    auto key = tie(callStmt, destMthd);
    if (SummaryFlowFunctionCache.count(key)) {
      ++summaryFFCacheHit;
      return SummaryFlowFunctionCache.at(key);
    } else {
      ++summaryFFConstruction;
      auto ff = problem.getSummaryFlowFunction(callStmt, destMthd);
      return ff;
    }
  }

  shared_ptr<EdgeFunction<V>> getNormalEdgeFunction(N curr, D currNode, N succ,
                                                    D succNode) {
    auto key = tie(curr, currNode, succ, succNode);
    if (NormalEdgeFunctionCache.count(key)) {
      ++normalEFCacheHit;
      return NormalEdgeFunctionCache.at(key);
    } else {
      ++normalEFConstruction;
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
      NormalEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  shared_ptr<EdgeFunction<V>>
  getCallEdgeFunction(N callStmt, D srcNode, M destiantionMethod, D destNode) {
    auto key = tie(callStmt, srcNode, destiantionMethod, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      ++callEFCacheHit;
      return CallEdgeFunctionCache.at(key);
    } else {
      ++callEFConstruction;
      auto ef = problem.getCallEdgeFunction(callStmt, srcNode,
                                            destiantionMethod, destNode);
      CallEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  shared_ptr<EdgeFunction<V>> getReturnEdgeFunction(N callSite, M calleeMethod,
                                                    N exitStmt, D exitNode,
                                                    N reSite, D retNode) {
    auto key = tie(callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    if (ReturnEdgeFunctionCache.count(key)) {
      ++returnEFCacheHit;
      return ReturnEdgeFunctionCache.at(key);
    } else {
      ++returnEFConstruction;
      auto ef = problem.getReturnEdgeFunction(callSite, calleeMethod, exitStmt,
                                              exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  shared_ptr<EdgeFunction<V>> getCallToReturnEdgeFunction(N callSite,
                                                          D callNode, N retSite,
                                                          D retSiteNode) {
    auto key = tie(callSite, callNode, retSite, retSiteNode);
    if (CallToRetEdgeFunctionCache.count(key)) {
      ++callToRetEFCacheHit;
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      ++callToRetEFConstruction;
      auto ef = problem.getCallToReturnEdgeFunction(callSite, callNode, retSite,
                                                    retSiteNode);
      CallToRetEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  shared_ptr<EdgeFunction<V>> getSummaryEdgeFunction(N callSite, D callNode,
                                                     N retSite, D retSiteNode) {
    auto key = tie(callSite, callNode, retSite, retSiteNode);
    if (SummaryEdgeFunctionCache.count(key)) {
      ++summaryEFCacheHit;
      return SummaryEdgeFunctionCache.at(key);
    } else {
      ++summaryEFConstruction;
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  void print() {
    cout << "Flow-Edge-Function Cache Statistics:\n"
         << "normal flow function cache hits: " << normalFFCacheHit << "\n"
         << "normal flow function constructions: " << normalFFConstruction
         << "\n"
         << "call flow function cache hits: " << callFFCacheHit << "\n"
         << "call flow function constructions: " << callFFConstruction << "\n"
         << "return flow function cache hits: " << returnFFCacheHit << "\n"
         << "return flow function constructions: " << returnFFConstruction
         << "\n"
         << "call to return flow function cache hits: " << callToRetFFCacheHit
         << "\n"
         << "call to return flow function constructions: "
         << callToRetFFConstruction << "\n"
         << "summary flow function cache hits: " << summaryFFCacheHit << "\n"
         << "summary flow function constructions: " << summaryFFConstruction
         << "\n"
         << "total flow function cache hits: "
         << normalFFCacheHit + callFFCacheHit + returnFFCacheHit +
                callToRetFFCacheHit + summaryFFCacheHit
         << "\n"
         << "total flow function constructions: "
         << normalFFConstruction + callFFConstruction + returnFFConstruction +
                callToRetFFConstruction + summaryFFConstruction
         << "\n"
         << "---"
         << "\n"
         << "normal edge function cache hits: " << normalEFCacheHit << "\n"
         << "normal edge function constructions: " << normalEFConstruction
         << "\n"
         << "call edge function cache hits: " << callEFCacheHit << "\n"
         << "call edge function constructions: " << callEFConstruction << "\n"
         << "return edge function cache hits: " << returnEFCacheHit << "\n"
         << "return edge function constructions: " << returnEFConstruction
         << "\n"
         << "call to return edge function cache hits: " << callToRetEFCacheHit
         << "\n"
         << "call to return edge function constructions: "
         << callToRetEFConstruction << "\n"
         << "summary edge function cache hits: " << summaryEFCacheHit << "\n"
         << "summary edge function constructions: " << summaryEFConstruction
         << "\n"
         << "total edge function cache hits: "
         << normalEFCacheHit + callEFCacheHit + returnEFCacheHit +
                callToRetEFCacheHit + summaryEFCacheHit
         << "\n"
         << "total edge function constructions: "
         << normalEFConstruction + callEFConstruction + returnEFConstruction +
                callToRetEFConstruction + summaryEFConstruction
         << "\n";
  }
};

#endif
