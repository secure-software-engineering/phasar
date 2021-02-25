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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFact.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/Utils/EquivalenceClassMap.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

#include "llvm/ADT/DenseMap.h"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>

namespace psr {
template <typename KeyT> class DefaultMapKeyCompressor {
public:
  using KeyType = KeyT;
  using CompressedType = KeyT;

  [[nodiscard]] inline CompressedType getCompressedID(KeyT Key) { return Key; }
};

template <typename... Ts> class MapKeyCompressorCombinator : public Ts... {
public:
  using Ts::getCompressedID...;
};

class LLVMMapKeyCompressor {
public:
  using KeyType = const llvm::Value *;
  using CompressedType = uint32_t;

  [[nodiscard]] inline CompressedType getCompressedID(KeyType Key) {
    auto Search = Map.find(Key);
    if (Search == Map.end()) {
      return Map.insert(std::make_pair(Key, Map.size() + 1)).first->getSecond();
    }
    return Search->getSecond();
  }

private:
  llvm::DenseMap<KeyType, CompressedType> Map{};
};

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

  using DTKeyCompressorType = std::conditional_t<
      std::is_base_of_v<llvm::Value, std::remove_pointer_t<d_t>>,
      LLVMMapKeyCompressor, DefaultMapKeyCompressor<d_t>>;
  using NTKeyCompressorType = std::conditional_t<
      std::is_base_of_v<llvm::Value, std::remove_pointer_t<n_t>>,
      LLVMMapKeyCompressor, DefaultMapKeyCompressor<n_t>>;

  using MapKeyCompressorType = std::conditional_t<
      std::is_same_v<NTKeyCompressorType, DTKeyCompressorType>,
      NTKeyCompressorType,
      MapKeyCompressorCombinator<NTKeyCompressorType, DTKeyCompressorType>>;

private:
  MapKeyCompressorType KeyCompressor;

  using EdgeFuncInstKey = uint64_t;
  using EdgeFuncNodeKey = std::conditional_t<
      std::is_base_of_v<llvm::Value, std::remove_pointer_t<d_t>>, uint64_t,
      std::pair<d_t, d_t>>;
  using InnerEdgeFunctionMapType =
      EquivalenceClassMap<EdgeFuncNodeKey, EdgeFunctionPtrType>;

  IDETabulationProblem<AnalysisDomainTy, Container> &problem;
  // Auto add zero
  bool autoAddZero;
  d_t zeroValue;

  struct NormalEdgeFlowData {
    NormalEdgeFlowData(FlowFunctionPtrType Val)
        : FlowFuncPtr(std::move(Val)), EdgeFunctionMap{} {}
    NormalEdgeFlowData(InnerEdgeFunctionMapType Map)
        : FlowFuncPtr(nullptr), EdgeFunctionMap{std::move(Map)} {}

    FlowFunctionPtrType FlowFuncPtr;
    InnerEdgeFunctionMapType EdgeFunctionMap;
  };

  // Caches for the flow/edge functions
  std::map<EdgeFuncInstKey, NormalEdgeFlowData> NormalFunctionCache;

  // Caches for the flow functions
  std::map<std::tuple<n_t, f_t>, FlowFunctionPtrType> CallFlowFunctionCache;
  std::map<std::tuple<n_t, f_t, n_t, n_t>, FlowFunctionPtrType>
      ReturnFlowFunctionCache;
  std::map<std::tuple<n_t, n_t, std::set<f_t>>, FlowFunctionPtrType>
      CallToRetFlowFunctionCache;
  // Caches for the edge functions
  std::map<std::tuple<n_t, d_t, f_t, d_t>, EdgeFunctionPtrType>
      CallEdgeFunctionCache;
  std::map<std::tuple<n_t, f_t, n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      ReturnEdgeFunctionCache;
  std::map<EdgeFuncInstKey, InnerEdgeFunctionMapType>
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
    auto Key = createEdgeFunctionInstKey(curr, succ);
    auto SearchNormalFlowFunction = NormalFunctionCache.find(Key);
    if (SearchNormalFlowFunction != NormalFunctionCache.end()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Normal-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      if (SearchNormalFlowFunction->second.FlowFuncPtr != nullptr) {
        return SearchNormalFlowFunction->second.FlowFuncPtr;
      } else {
        auto ff =
            (autoAddZero)
                ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                      problem.getNormalFlowFunction(curr, succ), zeroValue)
                : problem.getNormalFlowFunction(curr, succ);
        SearchNormalFlowFunction->second.FlowFuncPtr = ff;
        return ff;
      }
    } else {
      INC_COUNTER("Normal-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                          problem.getNormalFlowFunction(curr, succ), zeroValue)
                    : problem.getNormalFlowFunction(curr, succ);
      NormalFunctionCache.insert(std::make_pair(Key, NormalEdgeFlowData(ff)));
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
    auto Key = std::tie(callStmt, destFun);
    auto SearchCallFlowFunction = CallFlowFunctionCache.find(Key);
    if (SearchCallFlowFunction != CallFlowFunctionCache.end()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Call-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return SearchCallFlowFunction->second;
    } else {
      INC_COUNTER("Call-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff =
          (autoAddZero)
              ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                    problem.getCallFlowFunction(callStmt, destFun), zeroValue)
              : problem.getCallFlowFunction(callStmt, destFun);
      CallFlowFunctionCache.insert(std::make_pair(Key, ff));
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
    auto Key = std::tie(callSite, calleeFun, exitStmt, retSite);
    auto SearchReturnFlowFunction = ReturnFlowFunctionCache.find(Key);
    if (SearchReturnFlowFunction != ReturnFlowFunctionCache.end()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Return-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return SearchReturnFlowFunction->second;
    } else {
      INC_COUNTER("Return-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                          problem.getRetFlowFunction(callSite, calleeFun,
                                                     exitStmt, retSite),
                          zeroValue)
                    : problem.getRetFlowFunction(callSite, calleeFun, exitStmt,
                                                 retSite);
      ReturnFlowFunctionCache.insert(std::make_pair(Key, ff));
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
    auto Key = std::tie(callSite, retSite, callees);
    auto SearchCallToRetFlowFunction = CallToRetFlowFunctionCache.find(Key);
    if (SearchCallToRetFlowFunction != CallToRetFlowFunctionCache.end()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Flow function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("CallToRet-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return SearchCallToRetFlowFunction->second;
    } else {
      INC_COUNTER("CallToRet-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff =
          (autoAddZero)
              ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                    problem.getCallToRetFlowFunction(callSite, retSite,
                                                     callees),
                    zeroValue)
              : problem.getCallToRetFlowFunction(callSite, retSite, callees);
      CallToRetFlowFunctionCache.insert(std::make_pair(Key, ff));
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

    EdgeFuncInstKey OuterMapKey = createEdgeFunctionInstKey(curr, succ);
    auto SearchInnerMap = NormalFunctionCache.find(OuterMapKey);
    if (SearchInnerMap != NormalFunctionCache.end()) {
      auto SearchEdgeFunc = SearchInnerMap->second.EdgeFunctionMap.find(
          createEdgeFunctionNodeKey(currNode, succNode));
      if (SearchEdgeFunc != SearchInnerMap->second.EdgeFunctionMap.end()) {
        INC_COUNTER("Normal-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Edge function fetched from cache";
                      BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
        return SearchEdgeFunc->second;
      }
      INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);

      SearchInnerMap->second.EdgeFunctionMap.insert(
          createEdgeFunctionNodeKey(currNode, succNode), ef);

      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
    INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);

    NormalFunctionCache.try_emplace(
        OuterMapKey, NormalEdgeFlowData(InnerEdgeFunctionMapType{std::make_pair(
                         createEdgeFunctionNodeKey(currNode, succNode), ef)}));

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Edge function constructed";
                  BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    return ef;
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
    auto Key = std::tie(callStmt, srcNode, destinationFunction, destNode);
    auto SearchCallEdgeFunction = CallEdgeFunctionCache.find(Key);
    if (SearchCallEdgeFunction != CallEdgeFunctionCache.end()) {
      INC_COUNTER("Call-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return SearchCallEdgeFunction->second;
    } else {
      INC_COUNTER("Call-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallEdgeFunction(callStmt, srcNode,
                                            destinationFunction, destNode);
      CallEdgeFunctionCache.insert(std::make_pair(Key, ef));
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
    auto Key =
        std::tie(callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
    auto SearchReturnEdgeFunction = ReturnEdgeFunctionCache.find(Key);
    if (SearchReturnEdgeFunction != ReturnEdgeFunctionCache.end()) {
      INC_COUNTER("Return-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return SearchReturnEdgeFunction->second;
    } else {
      INC_COUNTER("Return-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getReturnEdgeFunction(
          callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(std::make_pair(Key, ef));
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

    EdgeFuncInstKey OuterMapKey = createEdgeFunctionInstKey(callSite, retSite);
    auto SearchInnerMap = CallToRetEdgeFunctionCache.find(OuterMapKey);
    if (SearchInnerMap != CallToRetEdgeFunctionCache.end()) {
      auto SearchEdgeFunc = SearchInnerMap->second.find(
          createEdgeFunctionNodeKey(callNode, retSiteNode));
      if (SearchEdgeFunc != SearchInnerMap->second.end()) {
        INC_COUNTER("CTR-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Edge function fetched from cache";
                      BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
        return SearchEdgeFunc->second;
      }
      INC_COUNTER("CTR-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallToRetEdgeFunction(callSite, callNode, retSite,
                                                 retSiteNode, callees);

      SearchInnerMap->second.insert(
          createEdgeFunctionNodeKey(callNode, retSiteNode), ef);

      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function constructed";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }

    INC_COUNTER("CTR-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto ef = problem.getCallToRetEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode, callees);

    CallToRetEdgeFunctionCache.emplace(
        OuterMapKey,
        InnerEdgeFunctionMapType{std::make_pair(
            createEdgeFunctionNodeKey(callNode, retSiteNode), ef)});
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Edge function constructed";
                  BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    return ef;
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
    auto Key = std::tie(callSite, callNode, retSite, retSiteNode);
    auto SearchSummaryEdgeFunction = SummaryEdgeFunctionCache.find(Key);
    if (SearchSummaryEdgeFunction != SummaryEdgeFunctionCache.end()) {
      INC_COUNTER("Summary-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Edge function fetched from cache";
                    BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return SearchSummaryEdgeFunction->second;
    } else {
      INC_COUNTER("Summary-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(std::make_pair(Key, ef));
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
  inline EdgeFuncInstKey createEdgeFunctionInstKey(n_t n1, n_t n2) {
    uint64_t val = 0;
    val |= KeyCompressor.getCompressedID(n1);
    val <<= 32;
    val |= KeyCompressor.getCompressedID(n2);
    return val;
  }

  inline EdgeFuncNodeKey createEdgeFunctionNodeKey(d_t d1, d_t d2) {
    if constexpr (std::is_base_of_v<llvm::Value, std::remove_pointer_t<d_t>>) {
      uint64_t val = 0;
      val |= KeyCompressor.getCompressedID(d1);
      val <<= 32;
      val |= KeyCompressor.getCompressedID(d2);
      return val;
    } else {
      return std::make_pair(d1, d2);
    }
  }
};

} // namespace psr

#endif
