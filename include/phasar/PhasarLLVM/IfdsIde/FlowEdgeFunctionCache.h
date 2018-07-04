/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <tuple>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/ZeroedFlowFunction.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/PAMM.h>


namespace psr {

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
  std::map<std::tuple<N, N, std::set<M>>, std::shared_ptr<FlowFunction<D>>>
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
    REG_COUNTER("Normal-FF Construction");
    REG_COUNTER("Normal-FF Cache Hit");
    // Counters for the call flow functions
    REG_COUNTER("Call-FF Construction");
    REG_COUNTER("Call-FF Cache Hit");
    // Counters for return flow functions
    REG_COUNTER("Return-FF Construction");
    REG_COUNTER("Return-FF Cache Hit");
    // Counters for the call to return flow functions
    REG_COUNTER("CallToRet-FF Construction");
    REG_COUNTER("CallToRet-FF Cache Hit");
    // Counters for the summary flow functions
    // REG_COUNTER("Summary-FF Construction");
    // REG_COUNTER("Summary-FF Cache Hit");
    // Counters for the normal edge functions
    REG_COUNTER("Normal-EF Construction");
    REG_COUNTER("Normal-EF Cache Hit");
    // Counters for the call edge functions
    REG_COUNTER("Call-EF Construction");
    REG_COUNTER("Call-EF Cache Hit");
    // Counters for the return edge functions
    REG_COUNTER("Return-EF Construction");
    REG_COUNTER("Return-EF Cache Hit");
    // Counters for the call to return edge functions
    REG_COUNTER("CallToRet-EF Construction");
    REG_COUNTER("CallToRet-EF Cache Hit");
    // Counters for the summary edge functions
    REG_COUNTER("Summary-EF Construction");
    REG_COUNTER("Summary-EF Cache Hit");
  }

  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) {
    PAMM_FACTORY;
    auto key = std::tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      INC_COUNTER("Normal-FF Cache Hit");
      return NormalFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-FF Construction");
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<D>>(
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
      INC_COUNTER("Call-FF Cache Hit");
      return CallFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-FF Construction");
      auto ff =
          (autoAddZero)
              ? std::make_shared<ZeroedFlowFunction<D>>(
                    problem.getCallFlowFunction(callStmt, destMthd), zeroValue)
              : problem.getCallFlowFunction(callStmt, destMthd);
      CallFlowFunctionCache.insert(std::make_pair(key, ff));
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getRetFlowFunction(N callSite, M calleeMthd,
                                                      N exitStmt, N retSite) {
    PAMM_FACTORY;
    auto key = std::tie(callSite, calleeMthd, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      INC_COUNTER("Return-FF Cache Hit");
      return ReturnFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-FF Construction");
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<D>>(
                          problem.getRetFlowFunction(callSite, calleeMthd,
                                                     exitStmt, retSite),
                          zeroValue)
                    : problem.getRetFlowFunction(callSite, calleeMthd, exitStmt,
                                                 retSite);
      ReturnFlowFunctionCache.insert(std::make_pair(key, ff));
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite, std::set<M> callees) {
    PAMM_FACTORY;
    auto key = std::tie(callSite, retSite, callees);
    if (CallToRetFlowFunctionCache.count(key)) {
      INC_COUNTER("CallToRet-FF Cache Hit");
      return CallToRetFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-FF Construction");
      auto ff =
          (autoAddZero)
              ? std::make_shared<ZeroedFlowFunction<D>>(
                    problem.getCallToRetFlowFunction(callSite, retSite,
                                                     callees),
                    zeroValue)
              : problem.getCallToRetFlowFunction(callSite, retSite, callees);
      CallToRetFlowFunctionCache.insert(std::make_pair(key, ff));
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N callStmt,
                                                          M destMthd) {
    PAMM_FACTORY;
    // INC_COUNTER("Summary-FF Construction");
    auto ff = problem.getSummaryFlowFunction(callStmt, destMthd);
    return ff;
  }

  std::shared_ptr<EdgeFunction<V>> getNormalEdgeFunction(N curr, D currNode,
                                                         N succ, D succNode) {
    PAMM_FACTORY;
    auto key = std::tie(curr, currNode, succ, succNode);
    if (NormalEdgeFunctionCache.count(key)) {
      INC_COUNTER("Normal-EF Cache Hit");
      return NormalEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-EF Construction");
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
      NormalEdgeFunctionCache.insert(std::make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>>
  getCallEdgeFunction(N callStmt, D srcNode, M destiantionMethod, D destNode) {
    PAMM_FACTORY;
    auto key = std::tie(callStmt, srcNode, destiantionMethod, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      INC_COUNTER("Call-EF Cache Hit");
      return CallEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-EF Construction");
      auto ef = problem.getCallEdgeFunction(callStmt, srcNode,
                                            destiantionMethod, destNode);
      CallEdgeFunctionCache.insert(std::make_pair(key, ef));
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
      INC_COUNTER("Return-EF Cache Hit");
      return ReturnEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-EF Construction");
      auto ef = problem.getReturnEdgeFunction(callSite, calleeMethod, exitStmt,
                                              exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(std::make_pair(key, ef));
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
      INC_COUNTER("CallToRet-EF Cache Hit");
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-EF Construction");
      auto ef = problem.getCallToReturnEdgeFunction(callSite, callNode, retSite,
                                                    retSiteNode);
      CallToRetEdgeFunctionCache.insert(std::make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>>
  getSummaryEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode) {
    PAMM_FACTORY;
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (SummaryEdgeFunctionCache.count(key)) {
      INC_COUNTER("Summary-EF Cache Hit");
      return SummaryEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Summary-EF Construction");
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(std::make_pair(key, ef));
      return ef;
    }
  }

  void print() {
#ifdef PERFORMANCE_EVA
    PAMM_FACTORY;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Flow-Edge-Function Cache Statistics:");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "normal flow function cache hits: "
                            << GET_COUNTER("Normal-FF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "normal flow function constructions: "
                            << GET_COUNTER("Normal-FF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call flow function cache hits: "
                            << GET_COUNTER("Call-FF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call flow function constructions: "
                            << GET_COUNTER("Call-FF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "return flow function cache hits: "
                            << GET_COUNTER("Return-FF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "return flow function constructions: "
                            << GET_COUNTER("Return-FF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call to return flow function cache hits: "
                            << GET_COUNTER("CallToRet-FF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call to return flow function constructions: "
                            << GET_COUNTER("CallToRet-FF Construction"));
    // LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "summary flow function cache hits: "
    //                        << GET_COUNTER("Summary-FF Cache Hit"));
    // LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "summary flow function constructions: "
    //                         << GET_COUNTER("Summary-FF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
        << "total flow function cache hits: "
        << GET_SUM_COUNT({"Normal-FF Cache Hit", "Call-FF Cache Hit",
                          "Return-FF Cache Hit", "CallToRet-FF Cache Hit"}));
    //"Summary-FF Cache Hit"});
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
        << "total flow function constructions: "
        << GET_SUM_COUNT({"Normal-FF Construction", "Call-FF Construction",
                          "Return-FF Construction",
                          "CallToRet-FF Construction" /*,
                "Summary-FF Construction"*/}));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << " ");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "normal edge function cache hits: "
                            << GET_COUNTER("Normal-EF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "normal edge function constructions: "
                            << GET_COUNTER("Normal-EF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call edge function cache hits: "
                            << GET_COUNTER("Call-EF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call edge function constructions: "
                            << GET_COUNTER("Call-EF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "return edge function cache hits: "
                            << GET_COUNTER("Return-EF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "return edge function constructions: "
                            << GET_COUNTER("Return-EF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call to return edge function cache hits: "
                            << GET_COUNTER("CallToRet-EF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "call to return edge function constructions: "
                            << GET_COUNTER("CallToRet-EF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "summary edge function cache hits: "
                            << GET_COUNTER("Summary-EF Cache Hit"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "summary edge function constructions: "
                            << GET_COUNTER("Summary-EF Construction"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
        << "total edge function cache hits: "
        << GET_SUM_COUNT({"Normal-EF Cache Hit", "Call-EF Cache Hit",
                          "Return-EF Cache Hit", "CallToRet-EF Cache Hit",
                          "Summary-EF Cache Hit"}));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
        << "total edge function constructions: "
        << GET_SUM_COUNT({"Normal-EF Construction", "Call-EF Construction",
                          "Return-EF Construction", "CallToRet-EF Construction",
                          "Summary-EF Construction"}));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "----------------------------------------------");
#endif
  }
};

} // namespace psr
