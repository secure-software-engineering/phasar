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

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

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
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Table.h>

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
  wali::wfa::WFA Query;
  wali::wfa::WFA Answer;
  wali::sem_elem_t SRElem;
  Table<N, D, std::map<N, std::set<D>>> incomingtab;

public:
  WPDSSolver(WPDSProblem<N, D, M, V, I> &P)
      : P(P), ICFG(P.interproceduralCFG()),
        // FIXME: use a FWPDS without witnesses for proof-of-concept
        // implementation
        PDS(new wali::wpds::fwpds::FWPDS()), ZeroValue(P.zeroValue()),
        AcceptingState(wali::getKey("__accept")), SRElem(nullptr) {
    PDSState = wali::getKey(ZeroValue);
    DKey[ZeroValue] = PDSState;
  }
  ~WPDSSolver() = default;

  virtual void solve(N n) {
    std::cout << "WPDSSolver::solve()\n";
    // Construct the PDS
    submitInitalSeeds();
    std::ofstream pdsfile("pds.dot");
    PDS->print_dot(pdsfile, true);
    pdsfile.flush();
    pdsfile.close();
    std::cout << "PRINTED DOT FILE" << std::endl;
    // test the SRElem
    wali::test_semelem_impl(SRElem);
    // Solve the PDS
    wali::Key node = wali::getKey(n);
    wali::sem_elem_t ret = nullptr;
    if (SearchDirection::FORWARD == P.getSearchDirection()) {
      std::cout << "FORWARD\n";
      doForwardSearch(Answer);
      std::cout << "PATH SUMMARY - COMPUTING THE WEIGHTS" << std::endl;
      Answer.path_summary();
      wali::wfa::Trans goal;
      // check all data-flow facts
      std::cout << "All D(s)" << std::endl;
      for (auto Entry : DKey) {
        if (Answer.find(Entry.second, node, AcceptingState, goal)) {
          std::cout << "FOUND ANSWER!" << std::endl;
          std::cout << llvmIRToString(Entry.first);
          goal.weight()->print(std::cout << " :--- weight ---: ");
          std::cout << " : --- "
                    << static_cast<EnvTrafoToSemElem<V> &>(*goal.weight())
                           .F->computeTarget(0);
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
    std::cout << "SOLVED\n";
  }

  void doForwardSearch(wali::wfa::WFA &Answer) {
    // // define our own attribute printer:
    // struct MyAttributePrinter : wali::wfa::DotAttributePrinter {
    //   const std::unordered_map<D, wali::Key> &DKey;
    //   const std::unordered_map<N, wali::Key> &NKey;
    //   MyAttributePrinter(std::unordered_map<D, wali::Key> DKey,
    //                      std::unordered_map<N, wali::Key> NKey)
    //       : DKey(DKey), NKey(NKey) {}
    //   void print_extra_attributes(wali::wfa::State const *state,
    //                               std::ostream &o) override {
    //     auto key = state->name();
    //     for (auto entry : DKey) {
    //       if (entry.second == key) {
    //         o << ", xlabel=\"" << llvmIRToString(entry.first) << "\"";
    //         return;
    //       }
    //     }
    //   }
    //   void print_extra_attributes(wali::wfa::ITrans const *trans,
    //                               std::ostream &o) override {
    //     auto key = trans->stack();
    //     for (auto entry : NKey) {
    //       if (entry.second == key) {
    //         std::string lbl = llvmIRToString(entry.first);
    //         o << ", xlabel=\"" << lbl.substr(0, lbl.find(", align 4")) <<
    //         "\""; return;
    //       }
    //     }
    //   }
    // };

    // Create an automaton to AcceptingState the configuration <PDSState,
    // main_entry>
    wali::Key main_entry =
        wali::getKey(&*ICFG.getMethod("main")->begin()->begin());
    (&*ICFG.getMethod("main")->begin()->begin())->print(llvm::outs());
    llvm::outs() << '\n';
    Query.addTrans(PDSState, main_entry, AcceptingState, SRElem->one());
    Query.set_initial_state(PDSState);
    Query.add_final_state(AcceptingState);
    Query.print(std::cout << "before poststar!\n");
    std::ofstream before("before.dot");
    Query.print_dot(before, true); //, new MyAttributePrinter(DKey, NKey));
    PDS->poststar(Query, Answer);
    Answer.print(std::cout << "after poststar!\n");
    std::ofstream after("after.dot");
    Answer.print_dot(after, true); //, new MyAttributePrinter(DKey, NKey));
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
        auto ZeroValue_k = wali::getKey(ZeroValue);
        DKey[ZeroValue] = ZeroValue_k;
        auto Value_k = wali::getKey(Value);
        DKey[Value] = Value_k;
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
      for (D d3 : res) {
        std::shared_ptr<EdgeFunction<V>> f =
            P.getNormalEdgeFunction(n, d2, m, d3);
        // TODO we need a EdgeFunction() to weight sem_elem_t conversion
        auto d2_k = wali::getKey(d2);
        DKey[d2] = d2_k;
        auto d3_k = wali::getKey(d3);
        DKey[d3] = d3_k;
        auto n_k = wali::getKey(n);
        auto m_k = wali::getKey(m);
        wali::ref_ptr<EnvTrafoToSemElem<V>> wptr;
        wptr = new EnvTrafoToSemElem<V>(f, static_cast<JoinLattice<V> &>(P));
        std::cout << "ADD NORMAL RULE: " << P.DtoString(d2) << " | "
                  << P.NtoString(n) << " --> " << P.DtoString(d3) << " | "
                  << P.DtoString(m) << ", " << *wptr << ")" << std::endl;
        PDS->add_rule(d2_k, n_k, d3_k, m_k, wptr);
        if (!SRElem.is_valid()) {
          SRElem = wptr;
        }
        propagate(d2, m, d3, nullptr, false);
      }
    }
  }

  void processCall(PathEdge<N, D> Edge) {
    std::cout << "processCall()\n";
    D d1 = Edge.factAtSource();
    N n = Edge.getTarget();
    D d2 = Edge.factAtTarget();
    std::set<N> returnSiteNs = ICFG.getReturnSitesOfCallAt(n);
    std::set<M> callees = ICFG.getCalleesOfCallAt(n);
    // for each possible callee
    for (M sCalledProcN : callees) { // still line 14
      // compute the call-flow function
      std::shared_ptr<FlowFunction<D>> flowFunction =
          P.getCallFlowFunction(n, sCalledProcN);
      std::cout << "CALL FF SOURCE: " << llvmIRToString(d2) << std::endl;
      std::set<D> res = flowFunction->computeTargets(d2);
      if (d2 == ZeroValue) {
        res.insert(ZeroValue); // FIXME
      }
      std::cout << "CALL FF TARGETS CONTAIN ZEROVALUE: " << res.count(ZeroValue)
                << std::endl;
      // for each callee's start point(s)
      std::set<N> startPointsOf = ICFG.getStartPointsOf(sCalledProcN);
      // if startPointsOf is empty, the called function is a declaration
      for (N sP : startPointsOf) {
        // for each result node of the call-flow function
        for (D d3 : res) {
          std::shared_ptr<EdgeFunction<V>> fcall =
              P.getCallEdgeFunction(n, d1, sCalledProcN, d3);
          auto d1_k = wali::getKey(d1);
          DKey[d1] = d1_k;
          auto d3_k = wali::getKey(d3);
          DKey[d3] = d3_k;
          auto n_k = wali::getKey(n);
          auto sP_k = wali::getKey(sP);
          wali::ref_ptr<EnvTrafoToSemElem<V>> wptr(new EnvTrafoToSemElem<V>(
              fcall, static_cast<JoinLattice<V> &>(P)));
          std::cout << "ADD CALL RULE: " << P.DtoString(d1) << ", "
                    << P.NtoString(n) << ", " << P.DtoString(d3) << ", "
                    << P.NtoString(sP) << ", " << *wptr << std::endl;
          for (auto retSiteN : returnSiteNs) {
            auto retSiteN_k = wali::getKey(retSiteN);
            PDS->add_rule(d1_k, n_k, d3_k, sP_k, retSiteN_k, wptr);
          }
          if (!SRElem.is_valid()) {
            SRElem = wptr;
          }
          std::cout << "ADD INCOMING" << std::endl;
          addIncoming(sP, d3, n, d2);
          // create initial self-loop
          propagate(d3, sP, d3, n, false);
        }
      }
    }
    // process intra-procedural flows along call-to-return flow functions
    for (N returnSiteN : returnSiteNs) {
      std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction =
          P.getCallToRetFlowFunction(n, returnSiteN, callees);
      std::set<D> returnFacts = callToReturnFlowFunction->computeTargets(d2);
      for (D d3 : returnFacts) {
        std::shared_ptr<EdgeFunction<V>> edgeFnE =
            P.getCallToRetEdgeFunction(n, d2, returnSiteN, d3, callees);
        auto d1_k = wali::getKey(d1);
        DKey[d1] = d1_k;
        auto d3_k = wali::getKey(d3);
        DKey[d3] = d3_k;
        auto n_k = wali::getKey(n);
        auto returnSiteN_k = wali::getKey(returnSiteN);
        wali::ref_ptr<EnvTrafoToSemElem<V>> wptr(new EnvTrafoToSemElem<V>(
            edgeFnE, static_cast<JoinLattice<V> &>(P)));
        std::cout << "ADD CALLTORET RULE: " << P.DtoString(d2) << " | "
                  << P.NtoString(n) << " --> " << P.DtoString(d3) << ", "
                  << P.NtoString(returnSiteN) << ", " << *wptr << std::endl;
        PDS->add_rule(d1_k, n_k, d3_k, returnSiteN_k, wptr);
        if (!SRElem.is_valid()) {
          SRElem = wptr;
        }
        propagate(d1, returnSiteN, d3, n, false);
      }
    }
  }

  void processExit(PathEdge<N, D> Edge) {
    std::cout << "processExit()\n";
    N n = Edge.getTarget();
    M methodThatNeedsSummary = ICFG.getMethodOf(n);
    D d1 = Edge.factAtSource();
    D d2 = Edge.factAtTarget();
    std::cout << "D1: " << llvmIRToString(d1) << " -- " << llvmIRToString(n)
              << " -- D2: " << llvmIRToString(d2) << std::endl;
    // for each of the method's start points, determine incoming calls
    std::set<N> startPointsOf = ICFG.getStartPointsOf(methodThatNeedsSummary);
    std::map<N, std::set<D>> inc;
    // for (N sP : startPointsOf) {
    //   for (auto entry : incoming(d1, sP)) {
    //     inc[entry.first] = std::set<D>{entry.second};
    //   }
    // }
    // FIXME get the callsite by hand
    if (methodThatNeedsSummary->getName() != "main") {
      auto main = n->getModule()->getFunction("main");
      for (auto &BB : *main) {
        for (auto &i : BB) {
          if (auto cs = llvm::dyn_cast<llvm::CallInst>(&i)) {
            std::cout << "FOUND CALLSITE: " << llvmIRToString(cs) << std::endl;
            inc.insert(std::make_pair(cs, std::set<D>{}));
          }
        }
      }
    }
    std::cout << "inc.size(): " << inc.size() << std::endl;
    // for each incoming-call value
    for (auto entry : inc) {
      N c = entry.first;
      for (N retSiteC : ICFG.getReturnSitesOfCallAt(c)) {
        std::cout << "STILL HERE" << std::endl;
        // compute return-flow function
        std::shared_ptr<FlowFunction<D>> returnFlowFunc =
            P.getRetFlowFunction(c, methodThatNeedsSummary, n, retSiteC);
        std::cout << "RET FF SOURCE VALUE: " << llvmIRToString(d2) << std::endl;
        std::set<D> targets = returnFlowFunc->computeTargets(d2);
        if (d2 == ZeroValue && methodThatNeedsSummary->getName() != "main") {
          targets.insert(ZeroValue); // FIXME
        }
        std::cout << "RET TARGETS size(): " << targets.size() << std::endl;
        // for each target value at the return site
        for (D d5 : targets) {
          std::shared_ptr<EdgeFunction<V>> f = P.getReturnEdgeFunction(
              c, methodThatNeedsSummary, n, d2, retSiteC, d5);
          auto d1_k = wali::getKey(d1);
          DKey[d1] = d1_k;
          auto d2_k = wali::getKey(d2);
          DKey[d2] = d2_k;
          auto d5_k = wali::getKey(d5);
          DKey[d5] = d5_k;
          auto n_k = wali::getKey(n);
          wali::ref_ptr<EnvTrafoToSemElem<V>> wptr(
              new EnvTrafoToSemElem<V>(f, static_cast<JoinLattice<V> &>(P)));
          std::cout << "ADD RET RULE: " << P.DtoString(d2) << ", "
                    << P.NtoString(n) << ", " << P.DtoString(d5) << ", "
                    << *wptr << std::endl;
          PDS->add_rule(d2_k, n_k, d5_k, wptr);
          if (!SRElem.is_valid()) {
            SRElem = wptr;
          }
          propagate(d2, retSiteC, d5, c, false);
        }
      }
    }
  }

protected:
  void addIncoming(N sP, D d3, N n, D d2) {
    incomingtab.get(sP, d3)[n].insert(d2);
  }

  std::map<N, std::set<D>> incoming(D d1, N sP) {
    return incomingtab.get(sP, d1);
  }
};

} // namespace psr

#endif
