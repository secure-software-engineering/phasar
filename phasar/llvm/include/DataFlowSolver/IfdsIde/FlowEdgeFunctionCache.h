/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_FLOWEDGEFUNCTIONCACHE_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_FLOWEDGEFUNCTIONCACHE_H_

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>

#include "llvm/ADT/DenseMap.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFact.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/Utils/EquivalenceClassMap.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

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

  IDETabulationProblem<AnalysisDomainTy, Container> &Problem;
  // Auto add zero
  bool AutoAddZero;
  d_t ZV;

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
      : Problem(Problem),
        AutoAddZero(Problem.getIFDSIDESolverConfig().autoAddZero()),
        ZV(Problem.getZeroValue()) {
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
    REG_COUNTER("Summary-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Summary-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
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

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Normal flow function factory call");
        PHASAR_LOG_LEVEL(DEBUG, "(N) Curr Inst : " << Problem.NtoString(Curr));
        PHASAR_LOG_LEVEL(DEBUG, "(N) Succ Inst : " << Problem.NtoString(Succ)));
    auto Key = createEdgeFunctionInstKey(Curr, Succ);
    auto SearchNormalFlowFunction = NormalFunctionCache.find(Key);
    if (SearchNormalFlowFunction != NormalFunctionCache.end()) {
      PHASAR_LOG_LEVEL(DEBUG, "Flow function fetched from cache");
      INC_COUNTER("Normal-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      if (SearchNormalFlowFunction->second.FlowFuncPtr != nullptr) {
        return SearchNormalFlowFunction->second.FlowFuncPtr;
      }
      auto FF = (AutoAddZero)
                    ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                          Problem.getNormalFlowFunction(Curr, Succ), ZV)
                    : Problem.getNormalFlowFunction(Curr, Succ);
      SearchNormalFlowFunction->second.FlowFuncPtr = FF;
      return FF;
    }
    INC_COUNTER("Normal-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto FF = (AutoAddZero)
                  ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                        Problem.getNormalFlowFunction(Curr, Succ), ZV)
                  : Problem.getNormalFlowFunction(Curr, Succ);
    NormalFunctionCache.insert(std::make_pair(Key, NormalEdgeFlowData(FF)));
    PHASAR_LOG_LEVEL(DEBUG, "Flow function constructed");

    return FF;
  }

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(PHASAR_LOG_LEVEL(DEBUG, "Call flow function factory call");
                   PHASAR_LOG_LEVEL(DEBUG, "(N) Call Stmt : "
                                               << Problem.NtoString(CallSite));
                   PHASAR_LOG_LEVEL(
                       DEBUG, "(F) Dest Fun : " << Problem.FtoString(DestFun)));
    auto Key = std::tie(CallSite, DestFun);
    auto SearchCallFlowFunction = CallFlowFunctionCache.find(Key);
    if (SearchCallFlowFunction != CallFlowFunctionCache.end()) {
      PHASAR_LOG_LEVEL(DEBUG, "Flow function fetched from cache");
      INC_COUNTER("Call-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return SearchCallFlowFunction->second;
    }
    INC_COUNTER("Call-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto FF = (AutoAddZero)
                  ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                        Problem.getCallFlowFunction(CallSite, DestFun), ZV)
                  : Problem.getCallFlowFunction(CallSite, DestFun);
    CallFlowFunctionCache.insert(std::make_pair(Key, FF));
    PHASAR_LOG_LEVEL(DEBUG, "Flow function constructed");
    return FF;
  }

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitInst, n_t RetSite) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Return flow function factory call");
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Call Site : " << Problem.NtoString(CallSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(F) Callee    : " << Problem.FtoString(CalleeFun));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Exit Stmt : " << Problem.NtoString(ExitInst));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Ret Site  : " << Problem.NtoString(RetSite)));
    auto Key = std::tie(CallSite, CalleeFun, ExitInst, RetSite);
    auto SearchReturnFlowFunction = ReturnFlowFunctionCache.find(Key);
    if (SearchReturnFlowFunction != ReturnFlowFunctionCache.end()) {
      PHASAR_LOG_LEVEL(DEBUG, "Flow function fetched from cache");
      INC_COUNTER("Return-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return SearchReturnFlowFunction->second;
    }
    INC_COUNTER("Return-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto FF = (AutoAddZero)
                  ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                        Problem.getRetFlowFunction(CallSite, CalleeFun,
                                                   ExitInst, RetSite),
                        ZV)
                  : Problem.getRetFlowFunction(CallSite, CalleeFun, ExitInst,
                                               RetSite);
    ReturnFlowFunctionCache.insert(std::make_pair(Key, FF));
    PHASAR_LOG_LEVEL(DEBUG, "Flow function constructed");
    return FF;
  }

  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               const std::set<f_t> Callees) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Call-to-Return flow function factory call");

        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Call Site : " << Problem.NtoString(CallSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Ret Site  : " << Problem.NtoString(RetSite));
        PHASAR_LOG_LEVEL(DEBUG, "(F) Callee's  : "); for (auto callee
                                                          : Callees) {
          PHASAR_LOG_LEVEL(DEBUG, "  " << Problem.FtoString(callee));
        };)
    auto Key = std::tie(CallSite, RetSite, Callees);
    auto SearchCallToRetFlowFunction = CallToRetFlowFunctionCache.find(Key);
    if (SearchCallToRetFlowFunction != CallToRetFlowFunctionCache.end()) {
      PHASAR_LOG_LEVEL(DEBUG, "Flow function fetched from cache");
      INC_COUNTER("CallToRet-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return SearchCallToRetFlowFunction->second;
    }
    INC_COUNTER("CallToRet-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto FF =
        (AutoAddZero)
            ? std::make_shared<ZeroedFlowFunction<d_t, Container>>(
                  Problem.getCallToRetFlowFunction(CallSite, RetSite, Callees),
                  ZV)
            : Problem.getCallToRetFlowFunction(CallSite, RetSite, Callees);
    CallToRetFlowFunctionCache.insert(std::make_pair(Key, FF));
    PHASAR_LOG_LEVEL(DEBUG, "Flow function constructed");
    return FF;
  }

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite, f_t DestFun) {
    // PAMM_GET_INSTANCE;
    // INC_COUNTER("Summary-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Summary flow function factory call");
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Call Stmt : " << Problem.NtoString(CallSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(F) Dest Mthd : " << Problem.FtoString(DestFun));
        PHASAR_LOG_LEVEL(DEBUG, ' '));
    auto FF = Problem.getSummaryFlowFunction(CallSite, DestFun);
    return FF;
  }

  EdgeFunctionPtrType getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                            d_t SuccNode) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Normal edge function factory call");
        PHASAR_LOG_LEVEL(DEBUG, "(N) Curr Inst : " << Problem.NtoString(Curr));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Curr Node : " << Problem.DtoString(CurrNode));
        PHASAR_LOG_LEVEL(DEBUG, "(N) Succ Inst : " << Problem.NtoString(Succ));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Succ Node : " << Problem.DtoString(SuccNode)));

    EdgeFuncInstKey OuterMapKey = createEdgeFunctionInstKey(Curr, Succ);
    auto SearchInnerMap = NormalFunctionCache.find(OuterMapKey);
    if (SearchInnerMap != NormalFunctionCache.end()) {
      auto SearchEdgeFunc = SearchInnerMap->second.EdgeFunctionMap.find(
          createEdgeFunctionNodeKey(CurrNode, SuccNode));
      if (SearchEdgeFunc != SearchInnerMap->second.EdgeFunctionMap.end()) {
        INC_COUNTER("Normal-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
        PHASAR_LOG_LEVEL(DEBUG, "Edge function fetched from cache");
        PHASAR_LOG_LEVEL(
            DEBUG, "Provide Edge Function: " << SearchEdgeFunc->second->str());
        return SearchEdgeFunc->second;
      }
      INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto EF = Problem.getNormalEdgeFunction(Curr, CurrNode, Succ, SuccNode);

      SearchInnerMap->second.EdgeFunctionMap.insert(
          createEdgeFunctionNodeKey(CurrNode, SuccNode), EF);

      PHASAR_LOG_LEVEL(DEBUG, "Edge function constructed");
      PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: " << EF->str());
      return EF;
    }
    INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto EF = Problem.getNormalEdgeFunction(Curr, CurrNode, Succ, SuccNode);

    NormalFunctionCache.try_emplace(
        OuterMapKey, NormalEdgeFlowData(InnerEdgeFunctionMapType{std::make_pair(
                         createEdgeFunctionNodeKey(CurrNode, SuccNode), EF)}));

    PHASAR_LOG_LEVEL(DEBUG, "Edge function constructed");
    PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: " << EF->str());
    return EF;
  }

  EdgeFunctionPtrType getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                          f_t DestinationFunction,
                                          d_t DestNode) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Call edge function factory call");
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Call Stmt : " << Problem.NtoString(CallSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Src Node  : " << Problem.DtoString(SrcNode));

        PHASAR_LOG_LEVEL(
            DEBUG, "(F) Dest Fun : " << Problem.FtoString(DestinationFunction));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Dest Node : " << Problem.DtoString(DestNode)));
    auto Key = std::tie(CallSite, SrcNode, DestinationFunction, DestNode);
    auto SearchCallEdgeFunction = CallEdgeFunctionCache.find(Key);
    if (SearchCallEdgeFunction != CallEdgeFunctionCache.end()) {
      INC_COUNTER("Call-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      PHASAR_LOG_LEVEL(DEBUG, "Edge function fetched from cache");
      PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: "
                                  << SearchCallEdgeFunction->second->str());
      return SearchCallEdgeFunction->second;
    }
    INC_COUNTER("Call-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto EF = Problem.getCallEdgeFunction(CallSite, SrcNode,
                                          DestinationFunction, DestNode);
    CallEdgeFunctionCache.insert(std::make_pair(Key, EF));
    PHASAR_LOG_LEVEL(DEBUG, "Edge function constructed");
    PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: " << EF->str());
    return EF;
  }

  EdgeFunctionPtrType getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                            n_t ExitInst, d_t ExitNode,
                                            n_t RetSite, d_t RetNode) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Return edge function factory call");
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Call Site : " << Problem.NtoString(CallSite));
        PHASAR_LOG_LEVEL(
            DEBUG, "(F) Callee    : " << Problem.FtoString(CalleeFunction));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Exit Stmt : " << Problem.NtoString(ExitInst));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Exit Node : " << Problem.DtoString(ExitNode));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Ret Site  : " << Problem.NtoString(RetSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Ret Node  : " << Problem.DtoString(RetNode)));
    auto Key = std::tie(CallSite, CalleeFunction, ExitInst, ExitNode, RetSite,
                        RetNode);
    auto SearchReturnEdgeFunction = ReturnEdgeFunctionCache.find(Key);
    if (SearchReturnEdgeFunction != ReturnEdgeFunctionCache.end()) {
      INC_COUNTER("Return-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      PHASAR_LOG_LEVEL(DEBUG, "Edge function fetched from cache");
      PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: "
                                  << SearchReturnEdgeFunction->second->str());
      return SearchReturnEdgeFunction->second;
    }
    INC_COUNTER("Return-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto EF = Problem.getReturnEdgeFunction(CallSite, CalleeFunction, ExitInst,
                                            ExitNode, RetSite, RetNode);
    ReturnEdgeFunctionCache.insert(std::make_pair(Key, EF));
    PHASAR_LOG_LEVEL(DEBUG, "Edge function constructed");
    PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: " << EF->str());
    return EF;
  }

  EdgeFunctionPtrType getCallToRetEdgeFunction(n_t CallSite, d_t CallNode,
                                               n_t RetSite, d_t RetSiteNode,
                                               const std::set<f_t> &Callees) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Call-to-Return edge function factory call");

        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Call Site : " << Problem.NtoString(CallSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Call Node : " << Problem.DtoString(CallNode));

        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Ret Site  : " << Problem.NtoString(RetSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Ret Node  : " << Problem.DtoString(RetSiteNode));
        PHASAR_LOG_LEVEL(DEBUG, "(F) Callee's  : "); for (auto callee
                                                          : Callees) {
          PHASAR_LOG_LEVEL(DEBUG, "  " << Problem.FtoString(callee));
        });

    EdgeFuncInstKey OuterMapKey = createEdgeFunctionInstKey(CallSite, RetSite);
    auto SearchInnerMap = CallToRetEdgeFunctionCache.find(OuterMapKey);
    if (SearchInnerMap != CallToRetEdgeFunctionCache.end()) {
      auto SearchEdgeFunc = SearchInnerMap->second.find(
          createEdgeFunctionNodeKey(CallNode, RetSiteNode));
      if (SearchEdgeFunc != SearchInnerMap->second.end()) {
        INC_COUNTER("CallToRet-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
        PHASAR_LOG_LEVEL(DEBUG, "Edge function fetched from cache");
        PHASAR_LOG_LEVEL(
            DEBUG, "Provide Edge Function: " << SearchEdgeFunc->second->str());
        return SearchEdgeFunc->second;
      }
      INC_COUNTER("CallToRet-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto EF = Problem.getCallToRetEdgeFunction(CallSite, CallNode, RetSite,
                                                 RetSiteNode, Callees);

      SearchInnerMap->second.insert(
          createEdgeFunctionNodeKey(CallNode, RetSiteNode), EF);

      PHASAR_LOG_LEVEL(DEBUG, "Edge function constructed");
      PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: " << EF->str());
      return EF;
    }

    INC_COUNTER("CallToRet-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto EF = Problem.getCallToRetEdgeFunction(CallSite, CallNode, RetSite,
                                               RetSiteNode, Callees);

    CallToRetEdgeFunctionCache.emplace(
        OuterMapKey,
        InnerEdgeFunctionMapType{std::make_pair(
            createEdgeFunctionNodeKey(CallNode, RetSiteNode), EF)});
    PHASAR_LOG_LEVEL(DEBUG, "Edge function constructed");
    PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: " << EF->str());
    return EF;
  }

  EdgeFunctionPtrType getSummaryEdgeFunction(n_t CallSite, d_t CallNode,
                                             n_t RetSite, d_t RetSiteNode) {
    PAMM_GET_INSTANCE;
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Summary edge function factory call");
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Call Site : " << Problem.NtoString(CallSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Call Node : " << Problem.DtoString(CallNode));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(N) Ret Site  : " << Problem.NtoString(RetSite));
        PHASAR_LOG_LEVEL(DEBUG,
                         "(D) Ret Node  : " << Problem.DtoString(RetSiteNode));
        PHASAR_LOG_LEVEL(DEBUG, ' '));
    auto Key = std::tie(CallSite, CallNode, RetSite, RetSiteNode);
    auto SearchSummaryEdgeFunction = SummaryEdgeFunctionCache.find(Key);
    if (SearchSummaryEdgeFunction != SummaryEdgeFunctionCache.end()) {
      INC_COUNTER("Summary-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      PHASAR_LOG_LEVEL(DEBUG, "Edge function fetched from cache");
      PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: "
                                  << SearchSummaryEdgeFunction->second->str());
      return SearchSummaryEdgeFunction->second;
    }
    INC_COUNTER("Summary-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    auto EF = Problem.getSummaryEdgeFunction(CallSite, CallNode, RetSite,
                                             RetSiteNode);
    SummaryEdgeFunctionCache.insert(std::make_pair(Key, EF));
    PHASAR_LOG_LEVEL(DEBUG, "Edge function constructed");
    PHASAR_LOG_LEVEL(DEBUG, "Provide Edge Function: " << EF->str());
    return EF;
  }

  void print() {
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      PAMM_GET_INSTANCE;
      PHASAR_LOG_LEVEL(INFO, "=== Flow-Edge-Function Cache Statistics ===");
      PHASAR_LOG_LEVEL(INFO, "Normal-flow function cache hits: "
                                 << GET_COUNTER("Normal-FF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Normal-flow function constructions: "
                                 << GET_COUNTER("Normal-FF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Call-flow function cache hits: "
                                 << GET_COUNTER("Call-FF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Call-flow function constructions: "
                                 << GET_COUNTER("Call-FF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Return-flow function cache hits: "
                                 << GET_COUNTER("Return-FF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Return-flow function constructions: "
                                 << GET_COUNTER("Return-FF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Call-to-Return-flow function cache hits: "
                                 << GET_COUNTER("CallToRet-FF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Call-to-Return-flow function constructions: "
                                 << GET_COUNTER("CallToRet-FF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Summary-flow function cache hits: "
                                 << GET_COUNTER("Summary-FF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Summary-flow function constructions: "
                                 << GET_COUNTER("Summary-FF Construction"));
      PHASAR_LOG_LEVEL(INFO,
                       "Total flow function cache hits: " << GET_SUM_COUNT(
                           {"Normal-FF Cache Hit", "Call-FF Cache Hit",
                            "Return-FF Cache Hit", "CallToRet-FF Cache Hit"}));
      //"Summary-FF Cache Hit"});
      PHASAR_LOG_LEVEL(INFO, "Total flow function constructions: "
          << GET_SUM_COUNT({"Normal-FF Construction", "Call-FF Construction",
                            "Return-FF Construction",
                            "CallToRet-FF Construction" /*,
                "Summary-FF Construction"*/}));
      PHASAR_LOG_LEVEL(INFO, ' ');
      PHASAR_LOG_LEVEL(INFO, "Normal edge function cache hits: "
                                 << GET_COUNTER("Normal-EF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Normal edge function constructions: "
                                 << GET_COUNTER("Normal-EF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Call edge function cache hits: "
                                 << GET_COUNTER("Call-EF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Call edge function constructions: "
                                 << GET_COUNTER("Call-EF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Return edge function cache hits: "
                                 << GET_COUNTER("Return-EF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Return edge function constructions: "
                                 << GET_COUNTER("Return-EF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Call-to-Return edge function cache hits: "
                                 << GET_COUNTER("CallToRet-EF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Call-to-Return edge function constructions: "
                                 << GET_COUNTER("CallToRet-EF Construction"));
      PHASAR_LOG_LEVEL(INFO, "Summary edge function cache hits: "
                                 << GET_COUNTER("Summary-EF Cache Hit"));
      PHASAR_LOG_LEVEL(INFO, "Summary edge function constructions: "
                                 << GET_COUNTER("Summary-EF Construction"));
      PHASAR_LOG_LEVEL(INFO,
                       "Total edge function cache hits: " << GET_SUM_COUNT(
                           {"Normal-EF Cache Hit", "Call-EF Cache Hit",
                            "Return-EF Cache Hit", "CallToRet-EF Cache Hit",
                            "Summary-EF Cache Hit"}));
      PHASAR_LOG_LEVEL(
          INFO, "Total edge function constructions: " << GET_SUM_COUNT(
                    {"Normal-EF Construction", "Call-EF Construction",
                     "Return-EF Construction", "CallToRet-EF Construction",
                     "Summary-EF Construction"}));
      PHASAR_LOG_LEVEL(INFO, "----------------------------------------------");
    } else {
      PHASAR_LOG_LEVEL(
          INFO, "Cache statistics only recorded on PAMM severity level: Full.");
    }
  }

private:
  inline EdgeFuncInstKey createEdgeFunctionInstKey(n_t Lhs, n_t Rhs) {
    uint64_t Val = 0;
    Val |= KeyCompressor.getCompressedID(Lhs);
    Val <<= 32;
    Val |= KeyCompressor.getCompressedID(Rhs);
    return Val;
  }

  inline EdgeFuncNodeKey createEdgeFunctionNodeKey(d_t Lhs, d_t Rhs) {
    if constexpr (std::is_base_of_v<llvm::Value, std::remove_pointer_t<d_t>>) {
      uint64_t Val = 0;
      Val |= KeyCompressor.getCompressedID(Lhs);
      Val <<= 32;
      Val |= KeyCompressor.getCompressedID(Rhs);
      return Val;
    } else {
      return std::make_pair(Lhs, Rhs);
    }
  }
};

} // namespace psr

#endif
