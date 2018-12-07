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

#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

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

namespace llvm {
class CallInst;
}

namespace psr {

template <typename N, typename D, typename M, typename V, typename I>
class WPDSSolver {
private:
  WPDSProblem<N, D, M, V, I> &P;
  I ICFG;
  std::unique_ptr<wali::wpds::WPDS> PDS;
  D ZeroValue;
  wali::Key PDSState;
  wali::Key AcceptingState;
  std::unordered_map<D, wali::Key> DKey;
  std::unordered_map<N, wali::Key> NKey;
  wali::wfa::WFA Query;
  wali::wfa::WFA Answer;
  wali::sem_elem_t SRElem;

public:
  WPDSSolver(WPDSProblem<N, D, M, V, I> &P)
      : P(P), ICFG(P.interproceduralCFG()),
        // FIXME: use a FWPDS without witnesses for proof-of-concept
        // implementation
        PDS(new wali::wpds::fwpds::FWPDS()), ZeroValue(P.zeroValue()),
        AcceptingState(wali::getKey("__accept")), SRElem(nullptr) {
    DKey[ZeroValue] = wali::getKey(reinterpret_cast<size_t>(ZeroValue));
    PDSState = DKey[ZeroValue];
  }
  ~WPDSSolver() = default;

  virtual void solve(N n) {
    std::cout << "WPDSSolver::solve()\n";
    submitInitalSeeds();
    // Solve the PDS
    wali::Key node = NKey.at(n);
    wali::sem_elem_t ret = nullptr;
    if (SearchDirection::FORWARD == P.getSearchDirection()) {
      std::cout << "FORWARD\n";
      doForwardSearch(Answer);

      Answer.path_summary();

      wali::wfa::Trans goal;
      // check all data-flow facts
      std::cout << "All D(s)" << std::endl;
      for (auto Entry : DKey) {
        if (Answer.find(Entry.second, NKey[n], AcceptingState, goal)) {
          std::cout << "FOUND ANSWER!" << std::endl;
          goal.weight()->print(std::cout << "--- weight ---: ");
          // std::cout << " : --- " <<
          // static_cast<EnvTrafoToSemElem<V>&>(*goal.weight()).F->computeTarget(0);
          std::cout << std::endl;
        }
      }

      // auto ret = SRElem->zero();
      // wali::wfa::TransSet tset;
      // wali::wfa::TransSet::iterator titer;
      // tset = Answer.match(PDSState, node);
      // for (titer = tset.begin(); titer != tset.end(); titer++) {
      //   wali::wfa::ITrans *t = *titer;
      //   wali::sem_elem_t tmp(Answer.getState(t->to())->weight());
      //   tmp = tmp->extend(t->weight());
      //   ret = ret->combine(tmp);
      // }

    } else {
      std::cout << "BACKWARD\n";
      // doBackwardSearch(node, Answer);

      // ret = SRElem->zero();

      // wali::wfa::TransSet tset;
      // wali::wfa::TransSet::iterator titer;
      // tset = Answer.match(PDSState, node);
      // for (titer = tset.begin(); titer != tset.end(); titer++) {
      //   wali::wfa::ITrans *t = *titer;
      //   if (!Answer.isFinalState(t->to())) continue;
      //   wali::sem_elem_t tmp(Answer.getState(t->to())->weight());
      //   ret = ret->combine(tmp);
      // }
    }
  }

  void doForwardSearch(wali::wfa::WFA &Answer) {
    // Create an automaton to AcceptingState the configuration <PDSState,
    // main_entry>
    wali::Key main_entry = NKey.at(&*ICFG.getMethod("main")->begin()->begin());
    (&*ICFG.getMethod("main")->begin()->begin())->print(llvm::outs());
    llvm::outs() << '\n';
    Query.addTrans(PDSState, main_entry, AcceptingState, SRElem->one());
    Query.set_initial_state(PDSState);
    Query.add_final_state(AcceptingState);
    Query.print(std::cout << "before poststar!\n");
    PDS->poststar(Query, Answer);
    Answer.print(std::cout << "after poststar!\n");
  }

  void doBackwardSearch(N n, wali::wfa::WFA &Answer) {
    // // Create an automaton to AcceptingState the configurations {n \Gamma^*}
    // // wali::wfa::WFA query;

    // // Find the set of all return points
    // wali::wpds::WpdsStackSymbols syms;
    // PDS->for_each(syms);

    // Query.addTrans(PDSState, node, AcceptingState, SRElem->one());

    // std::set<wali::Key>::iterator it;
    // for (it = syms.returnPoints.begin(); it != syms.returnPoints.end(); it++)
    // {
    //   Query.addTrans(AcceptingState, *it, AcceptingState, SRElem->one());
    // }

    // Query.set_initial_state(PDSState);
    // Query.add_final_state(AcceptingState);

    // PDS->prestar(Query, Answer);
  }

  void doBackwardSearch(std::vector<N> &n_stack, wali::wfa::WFA &Answer) {
    // assert(n_stack.size() > 0);
    // std::vector<wali::Key> node_stack;
    // node_stack.reserve(n_stack.size());
    // for (auto n : n_stack) {
    //   node_stack.push_back(NKey.at(n));
    // }

    // // Create an automaton to AcceptingState the configuration <PDSState,
    // // node_stack>
    // wali::wfa::WFA query;
    // wali::Key temp_from = PDSState;
    // wali::Key temp_to = wali::WALI_EPSILON;  // add initialization to skip
    // g++
    //                                          // warning. safe b/c of above
    //                                          // assertion.

    // for (size_t i = 0; i < node_stack.size(); i++) {
    //   std::stringstream ss;
    //   ss << "__tmp_state_" << i;
    //   temp_to = wali::getKey(ss.str());
    //   query.addTrans(temp_from, node_stack[i], temp_to, SRElem->one());
    //   temp_from = temp_to;
    // }

    // query.set_initial_state(PDSState);
    // query.add_final_state(temp_to);

    // PDS->prestar(query, Answer);
  }

  std::unordered_map<D, V> resultsAt(N stmt, bool stripZero = false) {
    return {};
  }

  V resultAt(N stmt, D fact) { return 0; }

  void submitInitalSeeds() {
    std::map<N, std::set<D>> InitialSeeds{
        {&*ICFG.getMethod("main")->begin()->begin(), std::set<D>{ZeroValue}}};
    for (const auto &Seed : InitialSeeds) {
      N StartPoint = Seed.first;
      for (const D &Value : Seed.second) {
        // generate rule for initial seed
        DKey[ZeroValue] = wali::getKey(reinterpret_cast<size_t>(ZeroValue));
        DKey[Value] = wali::getKey(reinterpret_cast<size_t>(Value));
        NKey[StartPoint] = wali::getKey(reinterpret_cast<size_t>(StartPoint));
        wali::ref_ptr<EnvTrafoToSemElem<V>> wptr = new EnvTrafoToSemElem<V>(
            EdgeIdentity<V>::getInstance(), static_cast<JoinLattice<V> &>(P));
        PDS->add_rule(DKey[ZeroValue], NKey[StartPoint], DKey[Value],
                      NKey[StartPoint], wptr);
        // propagate facts along the ICFG
        propagate(ZeroValue, StartPoint, Value, nullptr, false);
      }
    }
  }

  void
  propagate(D sourceVal, N target, D targetVal,
            /* deliberately exposed to clients */ N relatedCallSite,
            /* deliberately exposed to clients */ bool isUnbalancedReturn) {
    PathEdge<N, D> Edge(sourceVal, target, targetVal);
    pathEdgeProcessingTask(Edge);
  }

  void pathEdgeProcessingTask(PathEdge<N, D> Edge) {
    bool isCall = ICFG.isCallStmt(Edge.getTarget());
    if (!isCall) {
      if (ICFG.isExitStmt(Edge.getTarget())) {
        processExit(Edge);
      }
      if (!ICFG.getSuccsOf(Edge.getTarget()).empty()) {
        processNormalFlow(Edge);
      }
    } else {
      processCall(Edge);
    }
  }

  void processNormalFlow(PathEdge<N, D> Edge) {
    std::cout << "processNormalFlow()\n";
    D d1 = Edge.factAtSource();
    N n = Edge.getTarget();
    D d2 = Edge.factAtTarget();
    auto SuccessorInst = ICFG.getSuccsOf(n);
    for (auto m : SuccessorInst) {
      std::shared_ptr<FlowFunction<D>> flowFunction =
          P.getNormalFlowFunction(n, m);
      std::set<D> res = flowFunction->computeTargets(d2);

      // std::cout << "SRC: " << P.DtoString(d2) << std::endl;
      // std::cout << "RES: ";
      // for (auto r : res) {
      //   std::cout << P.DtoString(r) << " || ";
      // }
      // std::cout << std::endl;

      for (D d3 : res) {
        std::shared_ptr<EdgeFunction<V>> f =
            P.getNormalEdgeFunction(n, d2, m, d3);
        // TODO we need a EdgeFunction() to weight sem_elem_t conversion
        DKey[d2] = wali::getKey(reinterpret_cast<size_t>(d2));
        DKey[d3] = wali::getKey(reinterpret_cast<size_t>(d3));
        NKey[n] = wali::getKey(reinterpret_cast<size_t>(n));
        NKey[m] = wali::getKey(reinterpret_cast<size_t>(m));
        wali::ref_ptr<EnvTrafoToSemElem<V>> wptr;
        wptr = new EnvTrafoToSemElem<V>(f, static_cast<JoinLattice<V> &>(P));
        std::cout << "PDS rule: " << P.DtoString(d2) << " | " << P.NtoString(n)
                  << " --> " << P.DtoString(d3) << " | " << P.DtoString(m)
                  << ", " << *wptr << ")" << std::endl;
        PDS->add_rule(DKey[d2], NKey[n], DKey[d3], NKey[m], wptr);
        if (!SRElem.is_valid()) {
          SRElem = wptr;
        }
        propagate(d2, m, d3, nullptr, false);
      }
    }
  }

  void processCall(PathEdge<N, D> Edge) {
    std::cout << "processCall()\n";
    // D d1 = Edge.factAtSource();
    // N n = Edge.getTarget();
    // D d2 = Edge.factAtTarget();
    // std::set<N> returnSiteNs = ICFG.getReturnSitesOfCallAt(n);
    // std::cout << "returnSiteNs.size(): " << returnSiteNs.size() << '\n';
    // std::set<M> callees = ICFG.getCalleesOfCallAt(n);
    // // for each possible callee
    // for (M sCalledProcN : callees) {  // still line 14
    //   // compute the call-flow function
    //   std::shared_ptr<FlowFunction<D>> flowFunction =
    //       P.getCallFlowFunction(n, sCalledProcN);
    //   std::set<D> res = flowFunction->computeTargets(d1);
    //   // for each callee's start point(s)
    //   std::set<N> startPointsOf = ICFG.getStartPointsOf(sCalledProcN);
    //   // if startPointsOf is empty, the called function is a declaration
    //   for (N sP : startPointsOf) {
    //     // for each result node of the call-flow function
    //     for (D d3 : res) {
    //       std::shared_ptr<EdgeFunction<V>> fun =
    //           P.getCallEdgeFunction(n, d1, sCalledProcN, d3);
    //       DKey[d1] = wali::getKey(reinterpret_cast<size_t>(d1));
    //       DKey[d3] = wali::getKey(reinterpret_cast<size_t>(d3));
    //       NKey[n] = wali::getKey(reinterpret_cast<size_t>(n));
    //       NKey[sP] = wali::getKey(reinterpret_cast<size_t>(sP));
    //       NKey[*returnSiteNs.begin()] =
    //           wali::getKey(reinterpret_cast<size_t>(*returnSiteNs.begin()));
    //       wali::ref_ptr<EnvTrafoToSemElem<V>> wptr(
    //           new EnvTrafoToSemElem<V>(fun, static_cast<JoinLattice<V>
    //           &>(P)));
    //       // std::cout << "PDS->add_rule(" << DKey[d1] << ", " << NKey[n] <<
    //       ",
    //       // "
    //       //           << DKey[d3] << ", " << NKey[*returnSiteNs.begin()] <<
    //       ",
    //       //           "
    //       //           << *wptr << ")\n";
    //       SRElem = wptr;
    //       PDS->add_rule(DKey[d1], NKey[n], DKey[d3], NKey[sP],
    //                     NKey[*returnSiteNs.begin()], wptr);
    //       // create initial self-loop
    //       propagate(d3, sP, d3, EdgeIdentity<V>::getInstance(), n, false);
    //     }
    //   }
    // }
    // // process intra-procedural flows along call-to-return flow functions
    // for (N returnSiteN : returnSiteNs) {
    //   std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction =
    //       P.getCallToRetFlowFunction(n, returnSiteN, callees);
    //   std::set<D> returnFacts = callToReturnFlowFunction->computeTargets(d1);
    //   for (D d3 : returnFacts) {
    //     std::shared_ptr<EdgeFunction<V>> edgeFnE =
    //         P.getCallToRetEdgeFunction(n, d2, returnSiteN, d3, callees);
    //     DKey[d1] = wali::getKey(reinterpret_cast<size_t>(d1));
    //     DKey[d3] = wali::getKey(reinterpret_cast<size_t>(d3));
    //     NKey[n] = wali::getKey(reinterpret_cast<size_t>(n));
    //     NKey[returnSiteN] =
    //     wali::getKey(reinterpret_cast<size_t>(returnSiteN));
    //     wali::ref_ptr<EnvTrafoToSemElem<V>> wptr(new EnvTrafoToSemElem<V>(
    //         edgeFnE, static_cast<JoinLattice<V> &>(P)));
    //     //  std::cout << "PDS->add_rule(" << DKey[d1] << ", " << NKey[n] <<
    //     ", "
    //     //            << DKey[d3] << ", " << NKey[returnSiteN] << ", " <<
    //     *wptr
    //     //            << ")\n";
    //     SRElem = wptr;
    //     PDS->add_rule(DKey[d1], NKey[n], DKey[d3], NKey[returnSiteN], wptr);
    //     propagate(d1, returnSiteN, d3, edgeFnE, n, false);
    //   }
    // }
  }

  void processExit(PathEdge<N, D> Edge) {
    std::cout << "processExit()\n";
    // N n = Edge.getTarget();
    // M methodThatNeedsSummary = ICFG.getMethodOf(n);
    // D d1 = Edge.factAtSource();
    // D d2 = Edge.factAtTarget();
    // //   // for each return site
    // //   // const llvm::CallInst *CI = nullptr;
    // //   // auto Mod = n->getModule();
    // //   // llvm::outs() << *Mod << "\n";
    // //   // for (auto &BB : *Mod) {
    // //   //   for (auto &Inst : BB) {
    // //   //     if (const llvm::CallInst *CallI =
    // //   llvm::dyn_cast<llvm::CallInst>(&Inst)) {
    // //   //       CI = CallI;
    // //   //     }
    // //   //   }
    // //   // }
    // //   // std::cout << CI << std::endl;
    // //   // exit(1);
    // //   // for (N retSiteC : ICFG.getReturnSitesOfCallAt(CI)) {
    // //     // compute return-flow function
    // //     std::shared_ptr<FlowFunction<D>> retFunction =
    // //         P.getRetFlowFunction(nullptr, methodThatNeedsSummary, n,
    // //         nullptr);
    // //     // for each incoming-call value
    // //     std::set<D> targets =
    // //         retFunction->computeTargets(d1);
    // //     // for each target value at the return site
    // //     for (D d5 : targets) {
    // std::shared_ptr<EdgeFunction<V>> f = EdgeIdentity<V>::getInstance();
    // // P.getReturnEdgeFunction(nullptr, methodThatNeedsSummary, n, d1,
    // nullptr,
    // // d2);

    // DKey[d1] = wali::getKey(reinterpret_cast<size_t>(d1));
    // DKey[d2] = wali::getKey(reinterpret_cast<size_t>(d2));
    // NKey[n] = wali::getKey(reinterpret_cast<size_t>(n));
    // wali::ref_ptr<EnvTrafoToSemElem<V>> wptr(
    //     new EnvTrafoToSemElem<V>(f, static_cast<JoinLattice<V> &>(P)));
    // //       //  std::cout << "PDS->add_rule(" << DKey[d1] << ", " << NKey[n]
    // <<
    // //       ",
    // //       //  "
    // //       //            << DKey[d3] << ", " << NKey[returnSiteN] << ", "
    // <<
    // //       //            *wptr
    // //       //            << ")\n";

    // PDS->add_rule(DKey[d1], NKey[n], DKey[d2], wptr);
    // if (!SRElem.is_valid()) {
    //   SRElem = wptr;
    // }
    // //       // propagate(d1, retSiteC, d5, f, CI, false);
    // //     }
    // //   // }
  }
};

} // namespace psr

#endif
