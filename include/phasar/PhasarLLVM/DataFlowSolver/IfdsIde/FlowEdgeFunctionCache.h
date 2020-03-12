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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ZeroedFlowFunction.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

namespace psr {

/**
 * This class caches flow and edge functions to avoid their reconstruction.
 * When a flow or edge function must be applied to multiple times, a cached
 * version is used if existend, otherwise a new one is created and inserted
 * into the cache.
 */
template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class FlowEdgeFunctionCache {
private:
  IDETabulationProblem<N, D, F, T, V, L, I> &problem;
  // Auto add zero
  bool autoAddZero;
  D zeroValue;
  // Caches for the flow functions
  std::map<std::tuple<N, N>, std::shared_ptr<FlowFunction<D>>>
      NormalFlowFunctionCache;
  std::map<std::tuple<N, F>, std::shared_ptr<FlowFunction<D>>>
      CallFlowFunctionCache;
  std::map<std::tuple<N, F, N, N>, std::shared_ptr<FlowFunction<D>>>
      ReturnFlowFunctionCache;
  std::map<std::tuple<N, N, std::set<F>>, std::shared_ptr<FlowFunction<D>>>
      CallToRetFlowFunctionCache;
  // Caches for the edge functions
  std::map<std::tuple<N, D, N, D>, std::shared_ptr<EdgeFunction<L>>>
      NormalEdgeFunctionCache;
  std::map<std::tuple<N, D, F, D>, std::shared_ptr<EdgeFunction<L>>>
      CallEdgeFunctionCache;
  std::map<std::tuple<N, F, N, D, N, D>, std::shared_ptr<EdgeFunction<L>>>
      ReturnEdgeFunctionCache;
  std::map<std::tuple<N, D, N, D>, std::shared_ptr<EdgeFunction<L>>>
      CallToRetEdgeFunctionCache;
  std::map<std::tuple<N, D, N, D>, std::shared_ptr<EdgeFunction<L>>>
      SummaryEdgeFunctionCache;

public:
  // Ctor allows access to the IDEProblem in order to get access to flow and
  // edge function factory functions.
  FlowEdgeFunctionCache(IDETabulationProblem<N, D, F, T, V, L, I> &problem)
      : problem(problem),
        autoAddZero(problem.getIFDSIDESolverConfig().autoAddZero),
        zeroValue(problem.getZeroValue()) {
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

  ~FlowEdgeFunctionCache() = default;

  FlowEdgeFunctionCache(const FlowEdgeFunctionCache &FEFC) = default;

  FlowEdgeFunctionCache(FlowEdgeFunctionCache &&FEFC) = default;

  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Normal flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Curr Inst : " << problem.NtoString(curr));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Succ Inst : " << problem.NtoString(succ));
    auto key = std::tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      INC_COUNTER("Normal-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return NormalFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<D>>(
                          problem.getNormalFlowFunction(curr, succ), zeroValue)
                    : problem.getNormalFlowFunction(curr, succ);
      NormalFlowFunctionCache.insert(make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt, F destFun) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Call flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(F) Dest Fun : " << problem.FtoString(destFun));
    auto key = std::tie(callStmt, destFun);
    if (CallFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      INC_COUNTER("Call-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff =
          (autoAddZero)
              ? std::make_shared<ZeroedFlowFunction<D>>(
                    problem.getCallFlowFunction(callStmt, destFun), zeroValue)
              : problem.getCallFlowFunction(callStmt, destFun);
      CallFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getRetFlowFunction(N callSite, F calleeFun,
                                                      N exitStmt, N retSite) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Return flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(F) Callee    : " << problem.FtoString(calleeFun));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Exit Stmt : " << problem.NtoString(exitStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    auto key = std::tie(callSite, calleeFun, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      INC_COUNTER("Return-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return ReturnFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero) ? std::make_shared<ZeroedFlowFunction<D>>(
                                    problem.getRetFlowFunction(
                                        callSite, calleeFun, exitStmt, retSite),
                                    zeroValue)
                              : problem.getRetFlowFunction(callSite, calleeFun,
                                                           exitStmt, retSite);
      ReturnFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite, std::set<F> callees) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Call-to-Return flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "(F) Callee's  : ");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << problem.FtoString(callee));
    }
    auto key = std::tie(callSite, retSite, callees);
    if (CallToRetFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ff;
    }
  }

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N callStmt,
                                                          F destFun) {
    // PAMM_GET_INSTANCE;
    // INC_COUNTER("Summary-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Summary flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(F) Dest Mthd : " << problem.FtoString(destFun));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    auto ff = problem.getSummaryFlowFunction(callStmt, destFun);
    return ff;
  }

  std::shared_ptr<EdgeFunction<L>> getNormalEdgeFunction(N curr, D currNode,
                                                         N succ, D succNode) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Normal edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Curr Inst : " << problem.NtoString(curr));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Curr Node : " << problem.DtoString(currNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Succ Inst : " << problem.NtoString(succ));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Succ Node : " << problem.DtoString(succNode));
    auto key = std::tie(curr, currNode, succ, succNode);
    if (NormalEdgeFunctionCache.count(key)) {
      INC_COUNTER("Normal-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return NormalEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
      NormalEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<L>> getCallEdgeFunction(N callStmt, D srcNode,
                                                       F destinationFunction,
                                                       D destNode) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Call edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Src Node  : " << problem.DtoString(srcNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(F) Dest Fun : "
                  << problem.FtoString(destinationFunction));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Dest Node : " << problem.DtoString(destNode));
    auto key = std::tie(callStmt, srcNode, destinationFunction, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      INC_COUNTER("Call-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return CallEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallEdgeFunction(callStmt, srcNode,
                                            destinationFunction, destNode);
      CallEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<L>> getReturnEdgeFunction(N callSite,
                                                         F calleeFunction,
                                                         N exitStmt, D exitNode,
                                                         N reSite, D retNode) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Return edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(F) Callee    : " << problem.FtoString(calleeFunction));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Exit Stmt : " << problem.NtoString(exitStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Exit Node : " << problem.DtoString(exitNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(reSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retNode));
    auto key =
        std::tie(callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
    if (ReturnEdgeFunctionCache.count(key)) {
      INC_COUNTER("Return-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ReturnEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getReturnEdgeFunction(
          callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<L>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<F> callees) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Call-to-Return edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Call Node : " << problem.DtoString(callNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retSiteNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "(F) Callee's  : ");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << problem.FtoString(callee));
    }
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (CallToRetEdgeFunctionCache.count(key)) {
      INC_COUNTER("CallToRet-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallToRetEdgeFunction(callSite, callNode, retSite,
                                                 retSiteNode, callees);
      CallToRetEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return ef;
    }
  }

  std::shared_ptr<EdgeFunction<L>>
  getSummaryEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Summary edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Call Node : " << problem.DtoString(callNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retSiteNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (SummaryEdgeFunctionCache.count(key)) {
      INC_COUNTER("Summary-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return SummaryEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Summary-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << ' ');
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
