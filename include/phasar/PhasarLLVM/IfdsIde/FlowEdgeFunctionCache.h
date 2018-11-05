/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWEDGEFUNCTIONCACHE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWEDGEFUNCTIONCACHE_H_

#include <map>
#include <memory>
#include <set>
#include <tuple>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/ZeroedFlowFunction.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/PAMMMacros.h>

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
    PAMM_GET_INSTANCE;
    REG_COUNTER("Normal-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Normal-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call flow functions
    REG_COUNTER("Call-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Call-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for return flow functions
    REG_COUNTER("Return-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Return-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call to return flow functions
    REG_COUNTER("CallToRet-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("CallToRet-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the summary flow functions
    // REG_COUNTER("Summary-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    // REG_COUNTER("Summary-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the normal edge functions
    REG_COUNTER("Normal-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Normal-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call edge functions
    REG_COUNTER("Call-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Call-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the return edge functions
    REG_COUNTER("Return-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Return-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call to return edge functions
    REG_COUNTER("CallToRet-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("CallToRet-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the summary edge functions
    REG_COUNTER("Summary-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Summary-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
  }

  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) {
    PAMM_GET_INSTANCE;
    auto key = std::tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      INC_COUNTER("Normal-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return NormalFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<D>>(
                          problem.getNormalFlowFunction(curr, succ), zeroValue)
                    : problem.getNormalFlowFunction(curr, succ);
      NormalFlowFunctionCache.insert(make_pair(key, ff));
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt, M destMthd) {
    PAMM_GET_INSTANCE;
    auto key = std::tie(callStmt, destMthd);
    if (CallFlowFunctionCache.count(key)) {
      INC_COUNTER("Call-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
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
    PAMM_GET_INSTANCE;
    auto key = std::tie(callSite, calleeMthd, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      INC_COUNTER("Return-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return ReturnFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
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
    PAMM_GET_INSTANCE;
    auto key = std::tie(callSite, retSite, callees);
    if (CallToRetFlowFunctionCache.count(key)) {
      INC_COUNTER("CallToRet-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallToRetFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
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
    // PAMM_GET_INSTANCE;
    // INC_COUNTER("Summary-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto ff = problem.getSummaryFlowFunction(callStmt, destMthd);
    return ff;
  }

  std::shared_ptr<EdgeFunction<V>> getNormalEdgeFunction(N curr, D currNode,
                                                         N succ, D succNode) {
    PAMM_GET_INSTANCE;
    auto key = std::tie(curr, currNode, succ, succNode);
    if (NormalEdgeFunctionCache.count(key)) {
      INC_COUNTER("Normal-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return NormalEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
      NormalEdgeFunctionCache.insert(std::make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>>
  getCallEdgeFunction(N callStmt, D srcNode, M destiantionMethod, D destNode) {
    PAMM_GET_INSTANCE;
    auto key = std::tie(callStmt, srcNode, destiantionMethod, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      INC_COUNTER("Call-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
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
    PAMM_GET_INSTANCE;
    auto key =
        std::tie(callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    if (ReturnEdgeFunctionCache.count(key)) {
      INC_COUNTER("Return-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return ReturnEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getReturnEdgeFunction(callSite, calleeMethod, exitStmt,
                                              exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(std::make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<M> callees) {
    PAMM_GET_INSTANCE;
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (CallToRetEdgeFunctionCache.count(key)) {
      INC_COUNTER("CallToRet-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallToRetEdgeFunction(callSite, callNode, retSite,
                                                 retSiteNode, callees);
      CallToRetEdgeFunctionCache.insert(std::make_pair(key, ef));
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<V>>
  getSummaryEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode) {
    PAMM_GET_INSTANCE;
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (SummaryEdgeFunctionCache.count(key)) {
      INC_COUNTER("Summary-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return SummaryEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Summary-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(std::make_pair(key, ef));
      return ef;
    }
  }

  void print() {
    auto &lg = lg::get();
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      PAMM_GET_INSTANCE;
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "=== Flow-Edge-Function Cache Statistics ===");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Normal-flow function cache hits: "
                    << GET_COUNTER("Normal-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Normal-flow function constructions: "
                    << GET_COUNTER("Normal-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call-flow function cache hits: "
                    << GET_COUNTER("Call-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call-flow function constructions: "
                    << GET_COUNTER("Call-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Return-flow function cache hits: "
                    << GET_COUNTER("Return-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Return-flow function constructions: "
                    << GET_COUNTER("Return-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call-to-Return-flow function cache hits: "
                    << GET_COUNTER("CallToRet-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call-to-Return-flow function constructions: "
                    << GET_COUNTER("CallToRet-FF Construction"));
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Summary-flow function cache
      // hits: "
      //                        << GET_COUNTER("Summary-FF Cache Hit"));
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Summary-flow function
      // constructions: "
      //                         << GET_COUNTER("Summary-FF Construction"));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg, INFO)
          << "Total flow function cache hits: "
          << GET_SUM_COUNT({"Normal-FF Cache Hit", "Call-FF Cache Hit",
                            "Return-FF Cache Hit", "CallToRet-FF Cache Hit"}));
      //"Summary-FF Cache Hit"});
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg, INFO)
          << "Total flow function constructions: "
          << GET_SUM_COUNT({"Normal-FF Construction", "Call-FF Construction",
                            "Return-FF Construction",
                            "CallToRet-FF Construction" /*,
                "Summary-FF Construction"*/}));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << " ");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Normal edge function cache hits: "
                    << GET_COUNTER("Normal-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Normal edge function constructions: "
                    << GET_COUNTER("Normal-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call edge function cache hits: "
                    << GET_COUNTER("Call-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call edge function constructions: "
                    << GET_COUNTER("Call-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Return edge function cache hits: "
                    << GET_COUNTER("Return-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Return edge function constructions: "
                    << GET_COUNTER("Return-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call-to-Return edge function cache hits: "
                    << GET_COUNTER("CallToRet-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Call-to-Return edge function constructions: "
                    << GET_COUNTER("CallToRet-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Summary edge function cache hits: "
                    << GET_COUNTER("Summary-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Summary edge function constructions: "
                    << GET_COUNTER("Summary-EF Construction"));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg, INFO)
          << "Total edge function cache hits: "
          << GET_SUM_COUNT({"Normal-EF Cache Hit", "Call-EF Cache Hit",
                            "Return-EF Cache Hit", "CallToRet-EF Cache Hit",
                            "Summary-EF Cache Hit"}));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg, INFO)
          << "Total edge function constructions: "
          << GET_SUM_COUNT({"Normal-EF Construction", "Call-EF Construction",
                            "Return-EF Construction",
                            "CallToRet-EF Construction",
                            "Summary-EF Construction"}));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "----------------------------------------------");
    } else {
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg, INFO)
          << "Cache statistics only recorded on PAMM severity level: Full.");
    }
  }
};

} // namespace psr

#endif
