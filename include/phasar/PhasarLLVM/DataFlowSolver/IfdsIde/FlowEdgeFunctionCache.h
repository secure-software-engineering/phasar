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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

namespace psr {

/**
 * This class caches flow and edge functions to avoid their reconstruction.
 * When a flow or edge function must be applied to multiple times, a cached
 * version is used if existend, otherwise a new one is created and inserted
 * into the cache.
 */
template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class FlowEdgeFunctionCache {
  using IDEProblemType = IDETabulationProblem<AnalysisDomainTy, Container>;
  using FlowFunctionPtrType = typename IDEProblemType::FlowFunctionPtrType;
  using EdgeFunctionPtrType = typename IDEProblemType::EdgeFunctionPtrType;

  using n_t = typename AnalysisDomainTy::n_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;

private:
  using EdgeFuncInstKey = std::pair<n_t, n_t>;
  using EdgeFuncNodeKey = std::pair<d_t, d_t>;
  using InnerEdgeFunctionMapType =
      std::map<EdgeFuncNodeKey, EdgeFunctionPtrType>;

  IDETabulationProblem<AnalysisDomainTy, Container> &problem;
  // Auto add zero
  bool autoAddZero;
  d_t zeroValue;
  // Caches for the flow functions
  std::map<std::tuple<n_t, n_t>, FlowFunctionPtrType> NormalFlowFunctionCache;
  std::map<std::tuple<n_t, f_t>, FlowFunctionPtrType> CallFlowFunctionCache;
  std::map<std::tuple<n_t, f_t, n_t, n_t>, FlowFunctionPtrType>
      ReturnFlowFunctionCache;
  std::map<std::tuple<n_t, n_t, std::set<f_t>>, FlowFunctionPtrType>
      CallToRetFlowFunctionCache;
  // Caches for the edge functions
  std::map<EdgeFuncInstKey, InnerEdgeFunctionMapType> NormalEdgeFunctionCache;
  std::map<std::tuple<n_t, d_t, f_t, d_t>, EdgeFunctionPtrType>
      CallEdgeFunctionCache;
  std::map<std::tuple<n_t, f_t, n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      ReturnEdgeFunctionCache;
  std::map<std::tuple<n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      CallToRetEdgeFunctionCache;
  std::map<std::tuple<n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      SummaryEdgeFunctionCache;

public:
  // Ctor allows access to the IDEProblem in order to get access to flow and
  // edge function factory functions.
  FlowEdgeFunctionCache(
      IDETabulationProblem<AnalysisDomainTy, Container> &Problem)
      : problem(Problem),
        autoAddZero(problem.getIFDSIDESolverConfig().autoAddZero()),
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
  FlowEdgeFunctionCache &operator=(const FlowEdgeFunctionCache &FEFC) = default;

  FlowEdgeFunctionCache(FlowEdgeFunctionCache &&FEFC) noexcept = default;
  FlowEdgeFunctionCache &
  operator=(FlowEdgeFunctionCache &&FEFC) noexcept = default;

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Normal flow function factory call";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Curr Inst : " << problem.NtoString(curr);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Succ Inst : " << problem.NtoString(succ));
    auto key = std::tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Normal-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return NormalFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                          problem.getNormalFlowFunction(curr, succ), zeroValue)
                    : problem.getNormalFlowFunction(curr, succ);
      NormalFlowFunctionCache.insert(make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunctionPtrType getCallFlowFunction(n_t callStmt, f_t destFun) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Call flow function factory call";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Dest Fun : " << problem.FtoString(destFun));
    auto key = std::tie(callStmt, destFun);
    if (CallFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Call-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff =
          (autoAddZero)
              ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                    problem.getCallFlowFunction(callStmt, destFun), zeroValue)
              : problem.getCallFlowFunction(callStmt, destFun);
      CallFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeFun,
                                         n_t exitStmt, n_t retSite) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Return flow function factory call";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Callee    : " << problem.FtoString(calleeFun);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Exit Stmt : " << problem.NtoString(exitStmt);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    auto key = std::tie(callSite, calleeFun, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Return-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return ReturnFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                          problem.getRetFlowFunction(callSite, calleeFun,
                                                     exitStmt, retSite),
                          zeroValue)
                    : problem.getRetFlowFunction(callSite, calleeFun, exitStmt,
                                                 retSite);
      ReturnFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunctionPtrType getCallToRetFlowFunction(n_t callSite, n_t retSite,
                                               std::set<f_t> callees) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "Call-to-Return flow function factory call";
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(N) Call Site : " << problem.NtoString(callSite);
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(N) Ret Site  : " << problem.NtoString(retSite);
        BOOST_LOG_SEV(lg::get(), DEBUG) << "(F) Callee's  : "; for (auto callee
                                                                    : callees) {
          BOOST_LOG_SEV(lg::get(), DEBUG) << "  " << problem.FtoString(callee);
        });
    auto key = std::tie(callSite, retSite, callees);
    if (CallToRetFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("CallToRet-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallToRetFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff =
          (autoAddZero)
              ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                    problem.getCallToRetFlowFunction(callSite, retSite,
                                                     callees),
                    zeroValue)
              : problem.getCallToRetFlowFunction(callSite, retSite, callees);
      CallToRetFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunctionPtrType getSummaryFlowFunction(n_t callStmt, f_t destFun) {
    // PAMM_GET_INSTANCE;
    // INC_COUNTER("Summary-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Summary flow function factory call";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Dest Mthd : " << problem.FtoString(destFun);
                  BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    auto ff = problem.getSummaryFlowFunction(callStmt, destFun);
    return ff;
  }

  EdgeFunctionPtrType getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                                            d_t succNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Normal edge function factory call";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Curr Inst : " << problem.NtoString(curr);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Curr Node : " << problem.DtoString(currNode);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Succ Inst : " << problem.NtoString(succ);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Succ Node : " << problem.DtoString(succNode));
    if (hasNormalEdgeFunction(curr, currNode, succ, succNode)) {
      INC_COUNTER("Normal-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');

      EdgeFuncInstKey OuterMapKey = createEdgeFunctionInstKey(curr, succ);
      auto SearchInnerMap = NormalEdgeFunctionCache.find(OuterMapKey);
      assert(
          SearchInnerMap != NormalEdgeFunctionCache.end() &&
          "Outer map did not contain map node, which should be guaranteed by "
          "hasNormalEdgeFunction.");

      auto SearchEdgeFunc = SearchInnerMap->second.find(
          createEdgeFunctionNodeKey(currNode, succNode));
      assert(SearchEdgeFunc != SearchInnerMap->second.end() &&
             "Inner map did not contain EdgeFunction, which should be "
             "guaranteed by hasNormalEdgeFunction");

      return SearchEdgeFunc->second;
    } else {
      INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);

      EdgeFuncInstKey OuterMapKey = createEdgeFunctionInstKey(curr, succ);
      auto SearchInnerMap = NormalEdgeFunctionCache.find(OuterMapKey);
      if (SearchInnerMap != NormalEdgeFunctionCache.end()) {
        SearchInnerMap->second.emplace(
            createEdgeFunctionNodeKey(currNode, succNode), ef);
      } else {
        NormalEdgeFunctionCache.emplace(
            OuterMapKey,
            InnerEdgeFunctionMapType{
                {typename InnerEdgeFunctionMapType::value_type{
                    createEdgeFunctionNodeKey(currNode, succNode), ef}}});
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunctionPtrType getCallEdgeFunction(n_t callStmt, d_t srcNode,
                                          f_t destinationFunction,
                                          d_t destNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg::get(), DEBUG) << "Call edge function factory call";
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(N) Call Stmt : " << problem.NtoString(callStmt);
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(D) Src Node  : " << problem.DtoString(srcNode);
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(F) Dest Fun : " << problem.FtoString(destinationFunction);
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(D) Dest Node : " << problem.DtoString(destNode));
    auto key = std::tie(callStmt, srcNode, destinationFunction, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      INC_COUNTER("Call-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return CallEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallEdgeFunction(callStmt, srcNode,
                                            destinationFunction, destNode);
      CallEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunctionPtrType getReturnEdgeFunction(n_t callSite, f_t calleeFunction,
                                            n_t exitStmt, d_t exitNode,
                                            n_t reSite, d_t retNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Return edge function factory call";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Callee    : " << problem.FtoString(calleeFunction);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Exit Stmt : " << problem.NtoString(exitStmt);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Exit Node : " << problem.DtoString(exitNode);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(reSite);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retNode));
    auto key =
        std::tie(callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
    if (ReturnEdgeFunctionCache.count(key)) {
      INC_COUNTER("Return-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ReturnEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getReturnEdgeFunction(
          callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunctionPtrType getCallToRetEdgeFunction(n_t callSite, d_t callNode,
                                               n_t retSite, d_t retSiteNode,
                                               std::set<f_t> callees) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "Call-to-Return edge function factory call";
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(N) Call Site : " << problem.NtoString(callSite);
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(D) Call Node : " << problem.DtoString(callNode);
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(N) Ret Site  : " << problem.NtoString(retSite);
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "(D) Ret Node  : " << problem.DtoString(retSiteNode);
        BOOST_LOG_SEV(lg::get(), DEBUG) << "(F) Callee's  : "; for (auto callee
                                                                    : callees) {
          BOOST_LOG_SEV(lg::get(), DEBUG) << "  " << problem.FtoString(callee);
        });
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (CallToRetEdgeFunctionCache.count(key)) {
      INC_COUNTER("CallToRet-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallToRetEdgeFunction(callSite, callNode, retSite,
                                                 retSiteNode, callees);
      CallToRetEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunctionPtrType getSummaryEdgeFunction(n_t callSite, d_t callNode,
                                             n_t retSite, d_t retSiteNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Summary edge function factory call";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Call Node : " << problem.DtoString(callNode);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retSiteNode);
                  BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (SummaryEdgeFunctionCache.count(key)) {
      INC_COUNTER("Summary-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return SummaryEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Summary-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  void print() {
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      PAMM_GET_INSTANCE;
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "=== Flow-Edge-Function Cache Statistics ===");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal-flow function cache hits: "
                    << GET_COUNTER("Normal-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal-flow function constructions: "
                    << GET_COUNTER("Normal-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-flow function cache hits: "
                    << GET_COUNTER("Call-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-flow function constructions: "
                    << GET_COUNTER("Call-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return-flow function cache hits: "
                    << GET_COUNTER("Return-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return-flow function constructions: "
                    << GET_COUNTER("Return-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return-flow function cache hits: "
                    << GET_COUNTER("CallToRet-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return-flow function constructions: "
                    << GET_COUNTER("CallToRet-FF Construction"));
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << "Summary-flow function
      // cache hits: "
      //                        << GET_COUNTER("Summary-FF Cache Hit"));
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << "Summary-flow function
      // constructions: "
      //                         << GET_COUNTER("Summary-FF Construction"));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total flow function cache hits: "
          << GET_SUM_COUNT({"Normal-FF Cache Hit", "Call-FF Cache Hit",
                            "Return-FF Cache Hit", "CallToRet-FF Cache Hit"}));
      //"Summary-FF Cache Hit"});
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total flow function constructions: "
          << GET_SUM_COUNT({"Normal-FF Construction", "Call-FF Construction",
                            "Return-FF Construction",
                            "CallToRet-FF Construction" /*,
                "Summary-FF Construction"*/}));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << ' ');
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal edge function cache hits: "
                    << GET_COUNTER("Normal-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal edge function constructions: "
                    << GET_COUNTER("Normal-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call edge function cache hits: "
                    << GET_COUNTER("Call-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call edge function constructions: "
                    << GET_COUNTER("Call-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return edge function cache hits: "
                    << GET_COUNTER("Return-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return edge function constructions: "
                    << GET_COUNTER("Return-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return edge function cache hits: "
                    << GET_COUNTER("CallToRet-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return edge function constructions: "
                    << GET_COUNTER("CallToRet-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Summary edge function cache hits: "
                    << GET_COUNTER("Summary-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Summary edge function constructions: "
                    << GET_COUNTER("Summary-EF Construction"));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total edge function cache hits: "
          << GET_SUM_COUNT({"Normal-EF Cache Hit", "Call-EF Cache Hit",
                            "Return-EF Cache Hit", "CallToRet-EF Cache Hit",
                            "Summary-EF Cache Hit"}));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total edge function constructions: "
          << GET_SUM_COUNT({"Normal-EF Construction", "Call-EF Construction",
                            "Return-EF Construction",
                            "CallToRet-EF Construction",
                            "Summary-EF Construction"}));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "----------------------------------------------");
    } else {
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Cache statistics only recorded on PAMM severity level: Full.");
    }
  }

private:
  /// Checks if an EdgeFunction corresponding to the passed n_t/d_t values is
  /// cached.
  ///
  /// \returns true, if a cache entry is present
  inline bool hasNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                                    d_t succNode) {
    auto Search =
        NormalEdgeFunctionCache.find(createEdgeFunctionInstKey(curr, succ));
    if (Search != NormalEdgeFunctionCache.end()) {
      return Search->second.count(
          createEdgeFunctionNodeKey(currNode, succNode));
    }
    return false;
  }

  static inline EdgeFuncInstKey createEdgeFunctionInstKey(n_t n1, n_t n2) {
    return std::make_pair(n1, n2);
  }

  static inline EdgeFuncNodeKey createEdgeFunctionNodeKey(d_t d1, d_t d2) {
    return std::make_pair(d1, d2);
  }
};

} // namespace psr

#endif
