#ifndef FLOWEDGEFUNCTIONCACHE_HH_
#define FLOWEDGEFUNCTIONCACHE_HH_

#include <map>
#include <memory>
#include <tuple>
#include "EdgeFunction.hh"
#include "FlowFunction.hh"
#include "IDETabluationProblem.hh"
using namespace std;

/**
 * This class caches flow and edge functions and avoid their reconstruction.
 * When a flow or edge function must be applied to multiple times, a cached
 * version is used if existend, otherwise a new one is created and inserted
 * into the cache.
 */
template <typename N, typename D, typename M, typename V, typename I>
struct FlowEdgeFunctionCache {
  IDETabluationProblem<N, D, M, V, I> &problem;
  // Caches for the flow functions
  map<tuple<N, N>, shared_ptr<FlowFunction<D>>> NormalFlowFunctionCache;
  map<tuple<N, M>, shared_ptr<FlowFunction<D>>> CallFlowFunctionCache;
  map<tuple<N, M, N, N>, shared_ptr<FlowFunction<D>>> ReturnFlowFunctionCache;
  map<tuple<N, M, vector<D>, vector<bool>>, shared_ptr<FlowFunction<D>>>
      CallToRetFlowFunctionCache;
  // Caches for the edge functions
  map<tuple<N, D, N, D>, shared_ptr<EdgeFunction<V>>> NormalEdgeFunctionCache;
  map<tuple<N, D, M, D>, shared_ptr<EdgeFunction<V>>> CallEdgeFunctionCache;
  map<tuple<N, M, N, D, N, D>, shared_ptr<EdgeFunction<V>>>
      ReturnEdgeFunctionCache;
  map<tuple<N, M, vector<D>, vector<bool>>, shared_ptr<EdgeFunction<V>>>
      CallToRetEdgeFunctionCache;

  // Ctor allows access to the IDEProblem in order to get access to flow and
  // edge function factory functions
  FlowEdgeFunctionCache(IDETabluationProblem<N, D, M, V, I> &p) : problem(p) {}

  shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) {
    auto key = tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      return NormalFlowFunctionCache.at(key);
    } else {
      NormalFlowFunctionCache.insert(
          make_pair(key, problem.getNormalFlowFunction(curr, succ)));
      return NormalFlowFunctionCache.at(key);
    }
  }

  shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt, M destMthd) {
    auto key = tie(callStmt, destMthd);
    if (CallFlowFunctionCache.count(key)) {
      return CallFlowFunctionCache.at(key);
    } else {
      CallFlowFunctionCache.insert(
          make_pair(key, problem.getCallFlowFunction(callStmt, destMthd)));
      return CallFlowFunctionCache.at(key);
    }
  }

  shared_ptr<FlowFunction<D>> getReturnFlowFunction(N callSite, M calleeMthd,
                                                    N exitStmt, N retSite) {
    auto key = tie(callSite, calleeMthd, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      return ReturnFlowFunctionCache.at(key);
    } else {
      ReturnFlowFunctionCache.insert(
          make_pair(key, problem.getReturnFlowFunction(callSite, calleeMthd,
                                                       exitStmt, retSite)));
      return ReturnFlowFunctionCache.at(key);
    }
  }

  shared_ptr<FlowFunction<D>> getCallToRetFlowFunction(N callSite, N retSite) {
    auto key = tie(callSite, retSite);
    if (CallToRetFlowFunctionCache.count(key)) {
      return CallToRetFlowFunctionCache.at(key);
    } else {
      CallToRetFlowFunctionCache.insert(
          make_pair(key, problem.getCallToRetFlowFunction(callSite, retSite)));
      return CallToRetFlowFunctionCache.at(key);
    }
  }

  shared_ptr<FlowFunction<D>> getNormalEdgeFunction(N curr, D currNode, N succ,
                                                    D succNode) {
    auto key = tie(curr, currNode, succ, succNode);
    if (NormalEdgeFunctionCache.count(key)) {
      return NormalEdgeFunctionCache.at(key);
    } else {
      NormalEdgeFunctionCache.insert(make_pair(
          key, problem.getNormalEdgeFunction(curr, currNode, succ, succNode)));
      return NormalEdgeFunctionCache.at(key);
    }
  }

  shared_ptr<FlowFunction<D>> getCallEdgeFunction(N callStmt, D srcNode,
                                                  M destiantionMethod,
                                                  D destNode) {
    auto key = tie(callStmt, srcNode, destiantionMethod, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      return CallEdgeFunctionCache.at(key);
    } else {
      CallEdgeFunctionCache.insert(
          make_pair(key, problem.getCallEdgeFunction(
                             callStmt, srcNode, destiantionMethod, destNode)));
      return CallEdgeFunctionCache.at(key);
    }
  }

  shared_ptr<FlowFunction<D>> getReturnEdgeFunction(N callSite, M calleeMethod,
                                                    N exitStmt, D exitNode,
                                                    N reSite, D retNode) {
    auto key = tie(callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    if (ReturnEdgeFunctionCache.count(key)) {
      return ReturnEdgeFunctionCache.at(key);
    } else {
      ReturnEdgeFunctionCache.insert(make_pair(
          key, problem.getReturnEdgeFunction(callSite, calleeMethod, exitStmt,
                                             exitNode, reSite, retNode)));
      return ReturnEdgeFunctionCache.at(key);
    }
  }

  shared_ptr<FlowFunction<D>> getCallToRetEdgeFunction(N callSite, D callNode,
                                                       N retSite,
                                                       D retSiteNode) {
    auto key = tie(callSite, callNode, retSite, retSiteNode);
    if (CallToRetEdgeFunctionCache.count(key)) {
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      CallToRetEdgeFunctionCache.insert(
          make_pair(key, problem.getCallToRetEdgeFunction(
                             callSite, callNode, retSite, retSiteNode)));
      return CallToRetEdgeFunctionCache.at(key);
    }
  }
};

#endif
