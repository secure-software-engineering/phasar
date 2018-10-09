/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_SOLVER_WPDSSOLVER_H_
#define PHASAR_PHASARLLVM_WPDS_SOLVER_WPDSSOLVER_H_

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <wali/Common.hpp>
#include <wali/wfa/State.hpp>
#include <wali/wfa/WFA.hpp>
#include <wali/wpds/Rule.hpp>
#include <wali/wpds/RuleFunctor.hpp>
#include <wali/wpds/WPDS.hpp>
#include <wali/wpds/fwpds/FWPDS.hpp>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/PathEdge.h>
#include <phasar/PhasarLLVM/WPDS/EnvironmentTransformer.h>
#include <phasar/PhasarLLVM/WPDS/WPDSProblem.h>

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class WPDSSolver {
 private:
  WPDSProblem<N, D, M, V, I> &P;
  I ICFG;
  std::unique_ptr<wali::wpds::WPDS> PDS;
  D ZeroValue;
  wali::Key pds_state = wali::getKey("PDS_STATE");
  std::unordered_map<D, wali::Key> DKey;
  std::unordered_map<N, wali::Key> NKey;
  wali::wfa::WFA Question;
  wali::wfa::WFA Answer;
  wali::sem_elem_t se;

 public:
  WPDSSolver(WPDSProblem<N, D, M, V, I> &P)
      : P(P),
        ICFG(P.interproceduralCFG()),
        // FIXME: use a FWPDS without witnesses for proof-of-concept
        // implementation
        PDS(new wali::wpds::fwpds::FWPDS(false)),
        ZeroValue(P.zeroValue()) {}
  ~WPDSSolver() = default;

  virtual wali::sem_elem_t solve(N n) {
    std::cout << "WPDSSolver::solve()\n";
    submitInitalSeeds();
    // Solve the PDS
    wali::Key node = NKey.at(n);
    wali::sem_elem_t ret = nullptr;
    if (SearchDirection::FORWARD == P.getSearchDirection()) {
      std::cout << "FORWARD\n";
      doForwardSearch(Answer);
      Answer.path_summary();
      ret = se->zero();

      wali::wfa::TransSet tset;
      wali::wfa::TransSet::iterator titer;

      tset = Answer.match(pds_state, node);
      for (titer = tset.begin(); titer != tset.end(); titer++) {
        wali::wfa::ITrans *t = *titer;

        wali::sem_elem_t tmp(Answer.getState(t->to())->weight());

        tmp = tmp->extend(t->weight());
        ret = ret->combine(tmp);
      }
    } else {
      std::cout << "BACKWARD\n";
      doBackwardSearch(n, Answer);

      ret = se->zero();

      wali::wfa::TransSet tset;
      wali::wfa::TransSet::iterator titer;

      tset = Answer.match(pds_state, node);
      for (titer = tset.begin(); titer != tset.end(); titer++) {
        wali::wfa::ITrans *t = *titer;
        if (!Answer.isFinalState(t->to())) continue;

        wali::sem_elem_t tmp(Answer.getState(t->to())->weight());
        ret = ret->combine(tmp);
      }
    }
    std::cout << "Done!\n";
    return ret;
  }

  void doForwardSearch(wali::wfa::WFA &Answer) {
    // Create an automaton to accept the configuration <pds_state, main_entry>
    wali::wfa::WFA query;
    // Create an accepting state for the automaton
    wali::Key accept = wali::getKey("__accept");
    wali::Key main_entry = NKey.at(&*ICFG.getMethod("main")->begin()->begin());

    query.addTrans(pds_state, main_entry, accept, se->one());
    query.set_initial_state(pds_state);
    query.add_final_state(accept);

    PDS->poststar(query, Answer);
  }

  void doBackwardSearch(N n, wali::wfa::WFA &Answer) {
    // Create an automaton to accept the configurations {n \Gamma^*}
    wali::wfa::WFA query;

    // Create an accepting state for the automaton
    wali::Key accept = wali::getKey("__accept");

    // Find the set of all return points
    wali::wpds::WpdsStackSymbols syms;
    PDS->for_each(syms);

    query.addTrans(pds_state, NKey.at(n), accept, se->one());

    std::set<wali::Key>::iterator it;
    for (it = syms.returnPoints.begin(); it != syms.returnPoints.end(); it++) {
      query.addTrans(accept, *it, accept, se->one());
    }

    query.set_initial_state(pds_state);
    query.add_final_state(accept);

    PDS->prestar(query, Answer);
  }

  void doBackwardSearch(std::vector<N> &n_stack, wali::wfa::WFA &Answer) {
    assert(n_stack.size() > 0);
    std::vector<wali::Key> node_stack;
    node_stack.reserve(n_stack.size());
    for (auto n : n_stack) {
      node_stack.push_back(NKey.at(n));
    }

    // Create an automaton to accept the configuration <pds_state, node_stack>
    wali::wfa::WFA query;
    wali::Key temp_from = pds_state;
    wali::Key temp_to = wali::WALI_EPSILON;  // add initialization to skip g++
                                             // warning. safe b/c of above
                                             // assertion.

    for (size_t i = 0; i < node_stack.size(); i++) {
      std::stringstream ss;
      ss << "__tmp_state_" << i;
      temp_to = wali::getKey(ss.str());
      query.addTrans(temp_from, node_stack[i], temp_to, se->one());
      temp_from = temp_to;
    }

    query.set_initial_state(pds_state);
    query.add_final_state(temp_to);

    PDS->prestar(query, Answer);
  }

  void submitInitalSeeds() {
    std::map<N, std::set<D>> InitialSeeds{
        {&*ICFG.getMethod("main")->begin()->begin(), std::set<D>{ZeroValue}}};
    for (const auto &Seed : InitialSeeds) {
      N StartPoint = Seed.first;
      for (const D &Value : Seed.second) {
        propagate(ZeroValue, StartPoint, Value, EdgeIdentity<V>::getInstance(),
                  nullptr, false);
      }
    }
  }

  void propagate(
      D sourceVal, N target, D targetVal, std::shared_ptr<EdgeFunction<V>> f,
      /* deliberately exposed to clients */ N relatedCallSite,
      /* deliberately exposed to clients */ bool isUnbalancedReturn) {
    PathEdge<N, D> Edge(sourceVal, target, targetVal);
    pathEdgeProcessingTask(Edge, f);
  }

  void pathEdgeProcessingTask(PathEdge<N, D> Edge,
                              std::shared_ptr<EdgeFunction<V>> f) {
    bool isCall = ICFG.isCallStmt(Edge.getTarget());
    if (!isCall) {
      if (ICFG.isExitStmt(Edge.getTarget())) {
        processExit(Edge, f);
      }
      if (!ICFG.getSuccsOf(Edge.getTarget()).empty()) {
        processNormalFlow(Edge, f);
      }
    } else {
      processCall(Edge, f);
    }
  }

  void processNormalFlow(PathEdge<N, D> Edge,
                         std::shared_ptr<EdgeFunction<V>> f) {
    std::cout << "processNormalFlow()\n";
    D d1 = Edge.factAtSource();
    N n = Edge.getTarget();
    D d2 = Edge.factAtTarget();
    auto SuccessorInst = ICFG.getSuccsOf(n);
    for (auto m : SuccessorInst) {
      std::shared_ptr<FlowFunction<D>> flowFunction =
          P.getNormalFlowFunction(n, m);
      std::set<D> res = flowFunction->computeTargets(d1);
      for (D d3 : res) {
        std::shared_ptr<EdgeFunction<V>> g =
            P.getNormalEdgeFunction(n, d2, m, d3);
        // TODO we need a EdgeFunction() to weight sem_elem_t conversion
        DKey[d1] = wali::getKey(reinterpret_cast<size_t>(d1));
        DKey[d3] = wali::getKey(reinterpret_cast<size_t>(d3));
        NKey[n] = wali::getKey(reinterpret_cast<size_t>(n));
        NKey[m] = wali::getKey(reinterpret_cast<size_t>(m));
        wali::ref_ptr<EnvTrafoToSemElem<V>> wptr(
            new EnvTrafoToSemElem<V>(f, static_cast<JoinLattice<V> &>(P)));
        std::cout << "PDS->add_rule(" << DKey[d1] << ", " << NKey[n] << ", "
                  << DKey[d3] << ", " << NKey[m] << ", " << *wptr
                  << ")\n";
        se = wptr;
        PDS->add_rule(DKey[d1], NKey[n], DKey[d3], NKey[m], wptr);
        propagate(d1, m, d3, g, nullptr, false);
      }
    }
  }

  void processCall(PathEdge<N, D> Edge, std::shared_ptr<EdgeFunction<V>> f) {
    std::cout << "processCall()\n";
    // D d1 = edge.factAtSource();
    // N n = edge.getTarget();
    // D d2 = edge.factAtTarget();
    // std::set<N> returnSiteNs = icfg.getReturnSitesOfCallAt(n);
    // std::set<M> callees = icfg.getCalleesOfCallAt(n);
    // // for each possible callee
    // for (M sCalledProcN : callees) {  // still line 14
    //   // compute the call-flow function
    //   std::shared_ptr<FlowFunction<D>> function =
    //       P.getCallFlowFunction(n, sCalledProcN);
    //   std::set<D> res = function->computeTargets(d1);
    //   // for each callee's start point(s)
    //   std::set<N> startPointsOf = icfg.getStartPointsOf(sCalledProcN);
    //   // if startPointsOf is empty, the called function is a declaration
    //   for (N sP : startPointsOf) {
    //     // for each result node of the call-flow function
    //     for (D d3 : res) {
    //       PDS->add_rule(d1, n, d2, sP, *returnSiteNs.begin(), f);
    //       // create initial self-loop
    //       propagate(d3, sP, d3, EdgeIdentity<V>::getInstance(), n, false);
    //     }
    //   }
    // }
    // // // process intra-procedural flows along call-to-return flow functions
    // // for (N returnSiteN : returnSiteNs) {
    // //   std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction =
    // //       P.getCallToRetFlowFunction(n, returnSiteN, callees);
    // //   std::set<D> returnFacts =
    // callToReturnFlowFunction->computeTargets(d1);
    // //   for (D d3 : returnFacts) {
    // //     std::shared_ptr<EdgeFunction<V>> edgeFnE =
    // //         P.getCallToRetEdgeFunction(n, d2, returnSiteN, d3, callees);
    // //     propagate(d1, returnSiteN, d3, edgeFnE, n, false);
    // //   }
    // // }
  }

  void processExit(PathEdge<N, D> Edge, std::shared_ptr<EdgeFunction<V>> f) {
    std::cout << "processExit()\n";
    // N n = edge.getTarget();
    // M methodThatNeedsSummary = icfg.getMethodOf(n);
    // D d1 = edge.factAtSource();
    // D d2 = edge.factAtTarget();
    // // for each of the method's start points, determine incoming calls
    // std::set<N> startPointsOf =
    // icfg.getStartPointsOf(methodThatNeedsSummary);
    // ADD_TO_HIST("IDESolver", startPointsOf.size());
    // std::map<N, std::set<D>> inc;
    // for (N sP : startPointsOf) {
    //   // line 21.1 of Naeem/Lhotak/Rodriguez
    //   // register end-summary
    //   addEndSummary(sP, d1, n, d2, f);
    //   for (auto entry : incoming(d1, sP)) {
    //     inc[entry.first] = std::set<D>{entry.second};
    //     // ADD_TO_HIST("Data-flow facts", inc[entry.first].size());
    //   }
    // }
    // printEndSummaryTab();
    // printIncomingTab();
    // // for each incoming call edge already processed
    // //(see processCall(..))
    // for (auto entry : inc) {
    //   // line 22
    //   N c = entry.first;
    //   // for each return site
    //   for (N retSiteC : icfg.getReturnSitesOfCallAt(c)) {
    //     // compute return-flow function
    //     std::shared_ptr<FlowFunction<D>> retFunction =
    //         cachedFlowEdgeFunctions.getRetFlowFunction(
    //             c, methodThatNeedsSummary, n, retSiteC);
    //     INC_COUNTER("FF Queries");
    //     // for each incoming-call value
    //     for (D d4 : entry.second) {
    //       std::set<D> targets =
    //           computeReturnFlowFunction(retFunction, d1, d2, c,
    //           entry.second);
    //       ADD_TO_HIST("Data-flow facts", targets.size());
    //       saveEdges(n, retSiteC, d2, targets, true);
    //       // for each target value at the return site
    //       // line 23
    //       for (D d5 : targets) {
    //         // compute composed function
    //         // get call edge function
    //         std::shared_ptr<EdgeFunction<V>> f4 =
    //             cachedFlowEdgeFunctions.getCallEdgeFunction(
    //                 c, d4, icfg.getMethodOf(n), d1);
    //         // get return edge function
    //         std::shared_ptr<EdgeFunction<V>> f5 =
    //             cachedFlowEdgeFunctions.getReturnEdgeFunction(
    //                 c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
    //         INC_COUNTER_BY_VAL("EF Queries", 2);
    //         // compose call function * function * return function
    //         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
    //                       << "Compose: " << f5->str() << " * " << f->str()
    //                       << " * " << f4->str());
    //         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
    //                       << "         (return * function * call)");
    //         std::shared_ptr<EdgeFunction<V>> fPrime =
    //             f4->composeWith(f)->composeWith(f5);
    //         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
    //                       << "       = " << fPrime->str());
    //         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    //         // for each jump function coming into the call, propagate to
    //         return
    //         // site using the composed function
    //         for (auto valAndFunc : jumpFn->reverseLookup(c, d4)) {
    //           std::shared_ptr<EdgeFunction<V>> f3 = valAndFunc.second;
    //           if (!f3->equal_to(allTop)) {
    //             D d3 = valAndFunc.first;
    //             D d5_restoredCtx = restoreContextOnReturnedFact(c, d4, d5);
    //             LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
    //                           << "Compose: " << fPrime->str() << " * "
    //                           << f3->str());
    //             LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    //             propagate(d3, retSiteC, d5_restoredCtx,
    //             f3->composeWith(fPrime),
    //                       c, false);
    //           }
    //         }
    //       }
    //     }
    //   }
    // }
    // // handling for unbalanced problems where we return out of a method with
    // a
    // // fact for which we have no incoming flow.
    // // note: we propagate that way only values that originate from ZERO, as
    // // conditionally generated values should only
    // // be propagated into callers that have an incoming edge for this
    // condition
    // if (followReturnPastSeeds && inc.empty() &&
    //     ideTabulationProblem.isZeroValue(d1)) {
    //   std::set<N> callers = icfg.getCallersOf(methodThatNeedsSummary);
    //   ADD_TO_HIST("IDESolver", callers.size());
    //   for (N c : callers) {
    //     for (N retSiteC : icfg.getReturnSitesOfCallAt(c)) {
    //       std::shared_ptr<FlowFunction<D>> retFunction =
    //           cachedFlowEdgeFunctions.getRetFlowFunction(
    //               c, methodThatNeedsSummary, n, retSiteC);
    //       INC_COUNTER("FF Queries");
    //       std::set<D> targets = computeReturnFlowFunction(
    //           retFunction, d1, d2, c, std::set<D>{zeroValue});
    //       ADD_TO_HIST("Data-flow facts", targets.size());
    //       saveEdges(n, retSiteC, d2, targets, true);
    //       for (D d5 : targets) {
    //         std::shared_ptr<EdgeFunction<V>> f5 =
    //             cachedFlowEdgeFunctions.getReturnEdgeFunction(
    //                 c, icfg.getMethodOf(n), n, d2, retSiteC, d5);
    //         INC_COUNTER("EF Queries");
    //         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
    //                       << "Compose: " << f5->str() << " * " << f->str());
    //         LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    //         propagteUnbalancedReturnFlow(retSiteC, d5, f->composeWith(f5),
    //         c);
    //         // register for value processing (2nd IDE phase)
    //         unbalancedRetSites.insert(retSiteC);
    //       }
    //     }
    //   }
    //   // in cases where there are no callers, the return statement would
    //   // normally not be processed at all; this might be undesirable if
    //   // the flow function has a side effect such as registering a taint;
    //   // instead we thus call the return flow function will a null caller
    //   if (callers.empty()) {
    //     std::shared_ptr<FlowFunction<D>> retFunction =
    //         cachedFlowEdgeFunctions.getRetFlowFunction(
    //             nullptr, methodThatNeedsSummary, n, nullptr);
    //     INC_COUNTER("FF Queries");
    //     retFunction->computeTargets(d2);
    //   }
    // }
  }
};

}  // namespace psr

#endif
