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

#include "../../utils/PAMM.h"
#include "EdgeFunction.h"
#include "FlowFunction.h"
#include "IDETabulationProblem.h"
#include "ZeroedFlowFunction.h"
#include <map>
#include <memory>
#include <tuple>

/**
 * This class caches flow and edge functions to avoid their reconstruction.
 * When a flow or edge function must be applied to multiple times, a cached
 * version is used if existend, otherwise a new one is created and inserted
 * into the cache.
 */
template <typename N, typename D, typename M, typename V, typename I>
struct FlowEdgeFunctionCache {
  IDETabulationProblem<N, D, M, V, I> &problem;
  // Auto add zero
  bool autoAddZero;
  D zeroValue;
  // Caches for the flow functions
  std::map<std::tuple<N, N>, std::shared_ptr<FlowFunction<D>>>
      NormalFlowFunctionCache;
  std::map<std::tuple<N, M>, std::shared_ptr<FlowFunction<D>>>
      CallFlowFunctionCache;
  std::map<std::tuple<N, M, N, N>, std::shared_ptr<FlowFunction<D>>>
      ReturnFlowFunctionCache;
  std::map<std::tuple<N, N>, std::shared_ptr<FlowFunction<D>>>
      CallToRetFlowFunctionCache;
  // Caches for the edge functions
  std::map<std::tuple<N, D, N, D>, std::shared_ptr<EdgeFunction<V>>>
      NormalEdgeFunctionCache;
  std::map<std::tuple<N, D, M, D>, std::shared_ptr<EdgeFunction<V>>>
      CallEdgeFunctionCache;
  std::map<std::tuple<N, M, N, D, N, D>, std::shared_ptr<EdgeFunction<V>>>
      ReturnEdgeFunctionCache;
  std::map<std::tuple<N, D, N, D>, std::shared_ptr<EdgeFunction<V>>>
      CallToRetEdgeFunctionCache;
  std::map<std::tuple<N, D, N, D>, std::shared_ptr<EdgeFunction<V>>>
      SummaryEdgeFunctionCache;

  // Ctor allows access to the IDEProblem in order to get access to flow and
  // edge function factory functions.
  FlowEdgeFunctionCache(IDETabulationProblem<N, D, M, V, I> &problem)
      : problem(problem), autoAddZero(problem.solver_config.autoAddZero),
        zeroValue(problem.zeroValue()) {
    PAMM_FACTORY;
    REG_COUNTER("NormalFFConstructionCount");
    REG_COUNTER("NormalFFCacheHitCount");
    // Counters for the call flow functions
    REG_COUNTER("CallFFConstructionCount");
    REG_COUNTER("CallFFCacheHitCount");
    // Counters for return flow functions
    REG_COUNTER("ReturnFFConstructionCount");
    REG_COUNTER("ReturnFFCacheHitCount");
    // Counters for the call to return flow functions
    REG_COUNTER("CallToRetFFConstructionCount");
    REG_COUNTER("CallToRetFFCacheHitCount");
    // Counters for the summary flow functions
    REG_COUNTER("SummaryFFConstructionCount");
    //REG_COUNTER("SummaryFFCacheHitCount");
    // Counters for the normal edge functions
    REG_COUNTER("NormalEFConstructionCount");
    REG_COUNTER("NormalEFCacheHitCount");
    // Counters for the call edge functions
    REG_COUNTER("CallEFConstructionCount");
    REG_COUNTER("CallEFCacheHitCount");
    // Counters for the return edge functions
    REG_COUNTER("ReturnEFConstructionCount");
    REG_COUNTER("ReturnEFCacheHitCount");
    // Counters for the call to return edge functions
    REG_COUNTER("CallToRetEFConstructionCount");
    REG_COUNTER("CallToRetEFCacheHitCount");
    // Counters for the summary edge functions
    REG_COUNTER("SummaryEFConstructionCount");
    REG_COUNTER("SummaryEFCacheHitCount");
  }

  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) {
    PAMM_FACTORY;
    auto key = std::tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      INC_COUNTER("NormalFFCacheHitCount");
      return NormalFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("NormalFFConstructionCount");
      auto ff = (autoAddZero)
                    ? make_shared<ZeroedFlowFunction<D>>(
                          problem.getNormalFlowFunction(curr, succ), zeroValue)
                    : problem.getNormalFlowFunction(curr, succ);
      NormalFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt, M destMthd) {
    PAMM_FACTORY;
    auto key = std::tie(callStmt, destMthd);
    if (CallFlowFunctionCache.count(key)) {
      INC_COUNTER("CallFFCacheHitCount");
      return CallFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("CallFFConstructionCount");
      auto ff =
          (autoAddZero)
              ? make_shared<ZeroedFlowFunction<D>>(
                    problem.getCallFlowFunction(callStmt, destMthd), zeroValue)
              : problem.getCallFlowFunction(callStmt, destMthd);
      CallFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getRetFlowFunction(N callSite, M calleeMthd,
                                                      N exitStmt, N retSite) {
    PAMM_FACTORY;
    auto key = std::tie(callSite, calleeMthd, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      INC_COUNTER("ReturnFFCacheHitCount");
      return ReturnFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("ReturnFFConstructionCount");
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

  std::shared_ptr<FlowFunction<D>> getCallToRetFlowFunction(N callSite,
                                                            N retSite) {
    PAMM_FACTORY;
    auto key = std::tie(callSite, retSite);
    if (CallToRetFlowFunctionCache.count(key)) {
      INC_COUNTER("CallToRetFFCacheHitCount");
      return CallToRetFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRetFFConstructionCount");
      auto ff = (autoAddZero)
                    ? make_shared<ZeroedFlowFunction<D>>(
                          problem.getCallToRetFlowFunction(callSite, retSite),
                          zeroValue)
                    : problem.getCallToRetFlowFunction(callSite, retSite);
      CallToRetFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N callStmt,
                                                          M destMthd) {
    PAMM_FACTORY;
    INC_COUNTER("SummaryFFConstructionCount");
    auto ff = problem.getSummaryFlowFunction(callStmt, destMthd);
    return ff;
  }

  std::shared_ptr<EdgeFunction<V>> getNormalEdgeFunction(N curr, D currNode,
                                                         N succ, D succNode) {
    PAMM_FACTORY;
    auto key = std::tie(curr, currNode, succ, succNode);
    if (NormalEdgeFunctionCache.count(key)) {
      INC_COUNTER("NormalEFCacheHitCount");
      return NormalEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("NormalEFConstructionCount");
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
      NormalEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>>
  getCallEdgeFunction(N callStmt, D srcNode, M destiantionMethod, D destNode) {
    PAMM_FACTORY;
    auto key = std::tie(callStmt, srcNode, destiantionMethod, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      INC_COUNTER("CallEFCacheHitCount");
      return CallEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("CallEFConstructionCount");
      auto ef = problem.getCallEdgeFunction(callStmt, srcNode,
                                            destiantionMethod, destNode);
      CallEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>> getReturnEdgeFunction(N callSite,
                                                         M calleeMethod,
                                                         N exitStmt, D exitNode,
                                                         N reSite, D retNode) {
    PAMM_FACTORY;
    auto key =
        std::tie(callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    if (ReturnEdgeFunctionCache.count(key)) {
      INC_COUNTER("ReturnEFCacheHitCount");
      return ReturnEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("ReturnEFConstructionCount");
      auto ef = problem.getReturnEdgeFunction(callSite, calleeMethod, exitStmt,
                                              exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>> getCallToReturnEdgeFunction(N callSite,
                                                               D callNode,
                                                               N retSite,
                                                               D retSiteNode) {
    PAMM_FACTORY;
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (CallToRetEdgeFunctionCache.count(key)) {
      INC_COUNTER("CallToRetEFCacheHitCount");
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRetEFConstructionCount");
      auto ef = problem.getCallToReturnEdgeFunction(callSite, callNode, retSite,
                                                    retSiteNode);
      CallToRetEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>>
  getSummaryEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode) {
    PAMM_FACTORY;
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (SummaryEdgeFunctionCache.count(key)) {
      INC_COUNTER("SummaryEFCacheHitCount");
      return SummaryEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("SummaryEFConstructionCount");
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(make_pair(key, ef));
      return ef;
    }
  }

  void print() {
#ifdef PERFORMANCE_EVA
    PAMM_FACTORY;
    auto &lg = lg::get();
    BOOST_LOG_SEV(lg, INFO) << "Flow-Edge-Function Cache Statistics:";
    BOOST_LOG_SEV(lg, INFO) << "normal flow function cache hits: "
                            << GET_COUNTER("NormalFFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "normal flow function constructions: "
                            << GET_COUNTER("NormalFFConstructionCount");
    BOOST_LOG_SEV(lg, INFO) << "call flow function cache hits: "
                            << GET_COUNTER("CallFFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "call flow function constructions: "
                            << GET_COUNTER("CallFFConstructionCount");
    BOOST_LOG_SEV(lg, INFO) << "return flow function cache hits: "
                            << GET_COUNTER("ReturnFFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "return flow function constructions: "
                            << GET_COUNTER("ReturnFFConstructionCount");
    BOOST_LOG_SEV(lg, INFO) << "call to return flow function cache hits: "
                            << GET_COUNTER("CallToRetFFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "call to return flow function constructions: "
                            << GET_COUNTER("CallToRetFFConstructionCount");
    //BOOST_LOG_SEV(lg, INFO) << "summary flow function cache hits: "
    //                        << GET_COUNTER("SummaryFFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "summary flow function constructions: "
                            << GET_COUNTER("SummaryFFConstructionCount");
    BOOST_LOG_SEV(lg, INFO)
        << "total flow function cache hits: "
        << GET_SUM_COUNT({"NormalFFCacheHitCount", "CallFFCacheHitCount",
                          "ReturnFFCacheHitCount", "CallToRetFFCacheHitCount"});
                          //"SummaryFFCacheHitCount"});
    BOOST_LOG_SEV(lg, INFO)
        << "total flow function constructions: "
        << GET_SUM_COUNT(
               {"NormalFFConstructionCount", "CallFFConstructionCount",
                "ReturnFFConstructionCount", "CallToRetFFConstructionCount",
                "SummaryFFConstructionCount"});
    BOOST_LOG_SEV(lg, INFO) << " ";
    BOOST_LOG_SEV(lg, INFO) << "normal edge function cache hits: "
                            << GET_COUNTER("NormalEFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "normal edge function constructions: "
                            << GET_COUNTER("NormalEFConstructionCount");
    BOOST_LOG_SEV(lg, INFO) << "call edge function cache hits: "
                            << GET_COUNTER("CallEFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "call edge function constructions: "
                            << GET_COUNTER("CallEFConstructionCount");
    BOOST_LOG_SEV(lg, INFO) << "return edge function cache hits: "
                            << GET_COUNTER("ReturnEFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "return edge function constructions: "
                            << GET_COUNTER("ReturnEFConstructionCount");
    BOOST_LOG_SEV(lg, INFO) << "call to return edge function cache hits: "
                            << GET_COUNTER("CallToRetEFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "call to return edge function constructions: "
                            << GET_COUNTER("CallToRetEFConstructionCount");
    BOOST_LOG_SEV(lg, INFO) << "summary edge function cache hits: "
                            << GET_COUNTER("SummaryEFCacheHitCount");
    BOOST_LOG_SEV(lg, INFO) << "summary edge function constructions: "
                            << GET_COUNTER("SummaryEFConstructionCount");
    BOOST_LOG_SEV(lg, INFO)
        << "total edge function cache hits: "
        << GET_SUM_COUNT({"NormalEFCacheHitCount", "CallEFCacheHitCount",
                          "ReturnEFCacheHitCount", "CallToRetEFCacheHitCount",
                          "SummaryEFCacheHitCount"});
    BOOST_LOG_SEV(lg, INFO)
        << "total edge function constructions: "
        << GET_SUM_COUNT(
               {"NormalEFConstructionCount", "CallEFConstructionCount",
                "ReturnEFConstructionCount", "CallToRetEFConstructionCount",
                "SummaryEFConstructionCount"});
    BOOST_LOG_SEV(lg, INFO) << "----------------------------------------------";
#endif
  }
};

#endif
