/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_MEMORYMANAGER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_MEMORYMANAGER_H_

#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"

namespace psr {

template <typename AnalysisDomainTy,
          typename Container>
class FlowEdgeFunctionCache;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class MemoryManager {

    friend class FlowEdgeFunctionCache<AnalysisDomainTy,Container>;

    using IDEProblemType = IDETabulationProblem<AnalysisDomainTy, Container>;
    using FlowFunctionPtrType = typename IDEProblemType::FlowFunctionPtrType;
    using EdgeFunctionPtrType = typename IDEProblemType::EdgeFunctionPtrType;

    using n_t = typename AnalysisDomainTy::n_t;
    using d_t = typename AnalysisDomainTy::d_t;
    using f_t = typename AnalysisDomainTy::f_t;
    using t_t = typename AnalysisDomainTy::t_t;
    using l_t = typename AnalysisDomainTy::l_t;

    // Caches for the flow functions
    std::map<std::tuple<n_t, n_t>, FlowFunctionPtrType> NormalFlowFunctionCache;
    std::map<std::tuple<n_t, f_t>, FlowFunctionPtrType> CallFlowFunctionCache;
    std::map<std::tuple<n_t, f_t, n_t, n_t>, FlowFunctionPtrType>
      ReturnFlowFunctionCache;
    std::map<std::tuple<n_t, n_t, std::set<f_t>>, FlowFunctionPtrType>
      CallToRetFlowFunctionCache;
    // Caches for the edge functions
    std::map<std::tuple<n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      NormalEdgeFunctionCache;
    std::map<std::tuple<n_t, d_t, f_t, d_t>, EdgeFunctionPtrType>
      CallEdgeFunctionCache;
    std::map<std::tuple<n_t, f_t, n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      ReturnEdgeFunctionCache;
    std::map<std::tuple<n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      CallToRetEdgeFunctionCache;
    std::map<std::tuple<n_t, d_t, n_t, d_t>, EdgeFunctionPtrType>
      SummaryEdgeFunctionCache;

    // Data for clean up
    std::unordered_set<EdgeFunctionPtrType> managedEdgeFunctions;
    std::unordered_set<EdgeFunctionPtrType> registeredEdgeFunctionSingletons = {
      EdgeIdentity<l_t>::getInstance()};
    std::unordered_set<FlowFunctionPtrType> registeredFlowFunctionSingletons = {
      Identity<d_t>::getInstance(), KillAll<d_t>::getInstance()};

  std::unordered_set<FlowFunctionPtrType> managedFlowFunctions;


    ~MemoryManager(){
// Freeing all Flow Functions that are no singletons
    std::cout << "Cache destructor\n";
    for (auto elem : NormalFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : ReturnFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallToRetFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : managedFlowFunctions) {
      if (!registeredFlowFunctionSingletons.count(elem)) {
        delete elem;
      }
    }
    // Freeing all Edge Functions that are no singletons
    for (auto elem : NormalEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : ReturnEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallToRetEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : SummaryEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    // free additional edge functions
    for (auto elem : managedEdgeFunctions) {
      if (!registeredEdgeFunctionSingletons.count(elem)) {
        delete elem;
      }
    }
    }

    public:

    EdgeFunctionPtrType manageEdgeFunction(EdgeFunctionPtrType p) {
    managedEdgeFunctions.insert(p);
    return p;
  }

  FlowFunctionPtrType manageFlowFunction(FlowFunctionPtrType p){
    managedFlowFunctions.insert(p);
    return p;
  }

  void registerAsEdgeFunctionSingleton(std::set<EdgeFunctionPtrType> s) {
    registeredEdgeFunctionSingletons.insert(s.begin(), s.end());
  }

  void registerAsFlowFunctionSingleton(std::set<FlowFunctionPtrType> s) {
    registeredFlowFunctionSingletons.insert(s.begin(), s.end());
  }

};

} // namespace psr

#endif