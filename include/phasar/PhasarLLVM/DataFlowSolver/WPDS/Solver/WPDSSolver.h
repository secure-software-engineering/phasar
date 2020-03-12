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

#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include "wali/Common.hpp"
#include "wali/KeySource.hpp"
#include "wali/wfa/State.hpp"
#include "wali/wfa/WFA.hpp"
#include "wali/witness/WitnessWrapper.hpp"
#include "wali/wpds/Rule.hpp"
#include "wali/wpds/RuleFunctor.hpp"
#include "wali/wpds/WPDS.hpp"
#include "wali/wpds/fwpds/FWPDS.hpp"
#include "wali/wpds/fwpds/SWPDS.hpp"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/PathEdge.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/JoinLatticeToSemiRingElem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSProblem.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Table.h"

namespace llvm {
class CallInst;
}

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class WPDSSolver : public IDESolver<N, D, F, T, V, L, I> {
private:
  WPDSProblem<N, D, F, T, V, L, I> &Problem;
  WPDSSolverConfig SolverConf;
  std::vector<N> Stack;
  std::unique_ptr<wali::wpds::WPDS> PDS;
  D ZeroValue;
  wali::Key ZeroPDSState;
  wali::Key AcceptingState;
  std::unordered_map<D, wali::Key> DKey;
  wali::wfa::WFA Query;
  wali::wfa::WFA Answer;
  wali::sem_elem_t SRElem;
  Table<N, D, std::map<N, std::set<D>>> incomingtab;

  wali::wpds::WPDS *makePDS(WPDSType Ty, bool Witnesses) {
    wali::wpds::Wrapper *Wrapper =
        (Witnesses) ? new wali::witness::WitnessWrapper() : nullptr;
    switch (Ty) {
    case WPDSType::WPDS:
    case WPDSType::None:
      return new wali::wpds::WPDS(Wrapper);
      break;
    case WPDSType::EWPDS:
      return new wali::wpds::ewpds::EWPDS(Wrapper);
      break;
    case WPDSType::FWPDS:
      return new wali::wpds::fwpds::FWPDS(Wrapper);
      break;
    case WPDSType::SWPDS:
      return new wali::wpds::fwpds::SWPDS(Wrapper);
      break;
    case WPDSType::SYNCPDS:
      assert(false);
      break;
    }
  }

public:
  WPDSSolver(WPDSProblem<N, D, F, T, V, L, I> &Problem)
      : IDESolver<N, D, F, T, V, L, I>(Problem), Problem(Problem),
        SolverConf(Problem.getWPDSSolverConfig()),
        PDS(makePDS(SolverConf.wpdsty, SolverConf.recordWitnesses)),
        ZeroValue(Problem.getZeroValue()),
        AcceptingState(wali::getKey("__accept")), SRElem(nullptr) {
    ZeroPDSState = wali::getKey(ZeroValue);
    DKey[ZeroValue] = ZeroPDSState;
  }
  ~WPDSSolver() override = default;

  void solve() override {
    auto &lg = lg::get();
    // Construct the PDS
    IDESolver<N, D, F, T, V, L, I>::submitInitalSeeds();
    std::ofstream pdsfile("pds.dot");
    PDS->print_dot(pdsfile, true);
    pdsfile.flush();
    pdsfile.close();
    // test the SRElem
    wali::test_semelem_impl(SRElem);
    // Solve the PDS
    wali::sem_elem_t ret = nullptr;
    if (WPDSSearchDirection::FORWARD == SolverConf.searchDirection) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "FORWARD");
      doForwardSearch(Answer);
      Answer.path_summary();
      // another way not using path summary
      // wali::Key node = wali::getKey(n);
      // auto ret = SRElem->zero();
      // wali::wfa::TransSet tset;
      // wali::wfa::TransSet::iterator titer;
      // tset = Answer.match(ZeroPDSState, node);
      // for (titer = tset.begin(); titer != tset.end(); titer++) {
      //   wali::wfa::ITrans *t = *titer;
      //   wali::sem_elem_t tmp(Answer.getState(t->to())->weight());
      //   tmp = tmp->extend(t->weight());
      //   ret = ret->combine(tmp);
      // }
    } else {
      auto retnode =
          wali::getKey(&IDESolver<N, D, F, T, V, L, I>::ICF->getFunction("main")
                            ->back()
                            .back());
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "BACKWARD");
      doBackwardSearch(retnode, Answer);
      Answer.path_summary();

      // access the results
      // wali::wfa::Trans goal;
      // // check all data-flow facts
      // std::cout << "All D(s)" << std::endl;
      // for (auto Entry : DKey) {
      //   if (Answer.find(Entry.second, retnode, AcceptingState, goal)) {
      //     std::cout << "FOUND ANSWER!" << std::endl;
      //     std::cout << llvmIRToString(Entry.first);
      //     goal.weight()->print(std::cout << " :--- weight ---: ");
      //     std::cout << " : --- "
      //               << static_cast<JoinLatticeToSemiRingElem<L>
      //               &>(*goal.weight())
      //                      .F->computeTarget(L{});
      //     std::cout << std::endl;
      //   }
      // }

      // wali::sem_elem_t ret = SRElem->zero();
      // wali::wfa::TransSet tset;
      // wali::wfa::TransSet::iterator titer;
      // tset = Answer.match(a1key, retnode);
      // std::cout << "TRANSSET" << std::endl;
      // for (auto trans : tset) {
      //   trans->print(std::cout);
      //   std::cout << std::endl;
      // }
      // for (titer = tset.begin(); titer != tset.end(); titer++) {
      //   wali::wfa::ITrans *t = *titer;
      //   if (!Answer.isFinalState(t->to())) continue;
      //   wali::sem_elem_t tmp(Answer.getState(t->to())->weight());
      //   ret = ret->combine(tmp);
      // }
      // std::cout << "FOUND ANSWER!" << std::endl;
      // // std::cout << llvmIRToString(alloca1);
      // ret->print(std::cout << " :--- weight ---: ");
      // std::cout << " : --- "
      //           << static_cast<JoinLatticeToSemiRingElem<L>
      //           &>(*ret).F->computeTarget(L{});
      // std::cout << std::endl;
    }
  }

  void processNormalFlow(PathEdge<N, D> edge) override {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "WPDS::processNormal");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Normal", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process normal at target: "
                  << this->ideTabulationProblem.NtoString(edge.getTarget()));
    D d1 = edge.factAtSource();
    N n = edge.getTarget();
    D d2 = edge.factAtTarget();
    std::shared_ptr<EdgeFunction<L>> f =
        IDESolver<N, D, F, T, V, L, I>::jumpFunction(edge);
    auto successorInst = IDESolver<N, D, F, T, V, L, I>::ICF->getSuccsOf(n);
    for (auto f : successorInst) {
      std::shared_ptr<FlowFunction<D>> flowFunction =
          IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
              .getNormalFlowFunction(n, f);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      std::set<D> res =
          IDESolver<N, D, F, T, V, L, I>::computeNormalFlowFunction(
              flowFunction, d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      IDESolver<N, D, F, T, V, L, I>::saveEdges(n, f, d2, res, false);
      for (D d3 : res) {
        std::shared_ptr<EdgeFunction<L>> g =
            IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                .getNormalEdgeFunction(n, d2, f, d3);
        // add normal PDS rule
        auto d2_k = wali::getKey(d2);
        DKey[d2] = d2_k;
        auto d3_k = wali::getKey(d3);
        DKey[d3] = d3_k;
        auto n_k = wali::getKey(n);
        auto f_k = wali::getKey(f);
        wali::ref_ptr<JoinLatticeToSemiRingElem<L>> wptr;
        wptr = new JoinLatticeToSemiRingElem<L>(
            g, static_cast<JoinLattice<L> &>(Problem));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "ADD NORMAL RULE: " << Problem.DtoString(d2) << " | "
                      << Problem.NtoString(n) << " --> "
                      << Problem.DtoString(d3) << " | " << Problem.DtoString(f)
                      << ", " << *wptr << ")");
        PDS->add_rule(d2_k, n_k, d3_k, f_k, wptr);
        if (!SRElem.is_valid()) {
          SRElem = wptr;
        }
        std::shared_ptr<EdgeFunction<L>> fprime = f->composeWith(g);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Compose: " << g->str() << " * " << f->str());
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        IDESolver<N, D, F, T, V, L, I>::propagate(d1, f, d3, fprime, nullptr,
                                                  false);
      }
    }
  }

  void processCall(PathEdge<N, D> edge) override {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "WPDS::processCall");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Call", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process call at target: "
                  << this->ideTabulationProblem.NtoString(edge.getTarget()));
    D d1 = edge.factAtSource();
    N n = edge.getTarget(); // a call node; line 14...
    D d2 = edge.factAtTarget();
    std::shared_ptr<EdgeFunction<L>> f =
        IDESolver<N, D, F, T, V, L, I>::jumpFunction(edge);
    std::set<N> returnSiteNs =
        IDESolver<N, D, F, T, V, L, I>::ICF->getReturnSitesOfCallAt(n);
    std::set<F> callees =
        IDESolver<N, D, F, T, V, L, I>::ICF->getCalleesOfCallAt(n);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Possible callees:");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << callee->getName().str());
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Possible return sites:");
    for (auto ret : returnSiteNs) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << this->ideTabulationProblem.NtoString(ret));
    }
    // for each possible callee
    for (F sCalledProcN : callees) { // still line 14
      // check if a special summary for the called procedure exists
      std::shared_ptr<FlowFunction<D>> specialSum =
          IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
              .getSummaryFlowFunction(n, sCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (specialSum) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found and process special summary");
        for (N returnSiteN : returnSiteNs) {
          std::set<D> res =
              IDESolver<N, D, F, T, V, L, I>::computeSummaryFlowFunction(
                  specialSum, d1, d2);
          INC_COUNTER("SpecialSummary-FF Application", 1,
                      PAMM_SEVERITY_LEVEL::Full);
          ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<N, D, F, T, V, L, I>::saveEdges(n, returnSiteN, d2, res,
                                                    false);
          for (D d3 : res) {
            std::shared_ptr<EdgeFunction<L>> sumEdgFnE =
                IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                    .getSummaryEdgeFunction(n, d2, returnSiteN, d3);
            INC_COUNTER("SpecialSummary-EF Queries", 1,
                        PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << sumEdgFnE->str() << " * "
                          << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
            IDESolver<N, D, F, T, V, L, I>::propagate(
                d1, returnSiteN, d3, f->composeWith(sumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        std::shared_ptr<FlowFunction<D>> function =
            IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                .getCallFlowFunction(n, sCalledProcN);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<D> res =
            IDESolver<N, D, F, T, V, L, I>::computeCallFlowFunction(function,
                                                                    d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        // for each callee's start point(s)
        std::set<N> startPointsOf =
            IDESolver<N, D, F, T, V, L, I>::ICF->getStartPointsOf(sCalledProcN);
        if (startPointsOf.empty()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Start points of '" +
                               this->ICF->getFunctionName(sCalledProcN) +
                               "' currently not available!");
        }
        // if startPointsOf is empty, the called function is a declaration
        for (N sP : startPointsOf) {
          IDESolver<N, D, F, T, V, L, I>::saveEdges(n, sP, d2, res, true);
          // for each result node of the call-flow function
          for (D d3 : res) {
            // create initial self-loop
            IDESolver<N, D, F, T, V, L, I>::propagate(
                d3, sP, d3, EdgeIdentity<L>::getInstance(), n,
                false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            IDESolver<N, D, F, T, V, L, I>::addIncoming(sP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            std::set<
                typename Table<N, D, std::shared_ptr<EdgeFunction<L>>>::Cell>
                endSumm = std::set<typename Table<
                    N, D, std::shared_ptr<EdgeFunction<L>>>::Cell>(
                    IDESolver<N, D, F, T, V, L, I>::endSummary(sP, d3));
            // std::cout << "ENDSUMM" << std::endl;
            // std::cout << "Size: " << endSumm.size() << std::endl;
            // std::cout << "sP: " << ideTabulationProblem.NtoString(sP)
            //           << "\nd3: " << ideTabulationProblem.DtoString(d3)
            //           << std::endl;
            // printEndSummaryTab();
            // still line 15.2 of Naeem/Lhotak/Rodriguez
            // for each already-queried exit value <eP,d4> reachable from
            // <sP,d3>, create new caller-side jump functions to the return
            // sites because we have observed a potentially new incoming
            // edge into <sP,d3>
            for (typename Table<N, D, std::shared_ptr<EdgeFunction<L>>>::Cell
                     entry : endSumm) {
              N eP = entry.getRowKey();
              D d4 = entry.getColumnKey();
              std::shared_ptr<EdgeFunction<L>> fCalleeSummary =
                  entry.getValue();
              // for each return site
              for (N retSiteN : returnSiteNs) {
                // compute return-flow function
                std::shared_ptr<FlowFunction<D>> retFunction =
                    IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                        .getRetFlowFunction(n, sCalledProcN, eP, retSiteN);
                INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
                std::set<D> returnedFacts =
                    IDESolver<N, D, F, T, V, L, I>::computeReturnFlowFunction(
                        retFunction, d3, d4, n, std::set<D>{d2});
                ADD_TO_HISTOGRAM("Data-flow facts", returnedFacts.size(), 1,
                                 PAMM_SEVERITY_LEVEL::Full);
                IDESolver<N, D, F, T, V, L, I>::saveEdges(eP, retSiteN, d4,
                                                          returnedFacts, true);
                // for each target value of the function
                for (D d5 : returnedFacts) {
                  // update the caller-side summary function
                  // get call edge function
                  std::shared_ptr<EdgeFunction<L>> f4 =
                      IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                          .getCallEdgeFunction(n, d2, sCalledProcN, d3);
                  // add call PDS rule
                  auto d2_k = wali::getKey(d2);
                  DKey[d2] = d2_k;
                  auto d3_k = wali::getKey(d3);
                  DKey[d3] = d3_k;
                  auto n_k = wali::getKey(n);
                  auto sP_k = wali::getKey(sP);
                  wali::ref_ptr<JoinLatticeToSemiRingElem<L>> wptrCall(
                      new JoinLatticeToSemiRingElem<L>(
                          f4, static_cast<JoinLattice<L> &>(Problem)));
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "ADD CALL RULE: " << Problem.DtoString(d2)
                                << ", " << Problem.NtoString(n) << ", "
                                << Problem.DtoString(d3) << ", "
                                << Problem.NtoString(sP) << ", " << *wptrCall);
                  auto retSiteN_k = wali::getKey(retSiteN);
                  PDS->add_rule(d2_k, n_k, d3_k, sP_k, retSiteN_k, wptrCall);
                  if (!SRElem.is_valid()) {
                    SRElem = wptrCall;
                  }
                  // get return edge function
                  std::shared_ptr<EdgeFunction<L>> f5 =
                      IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                          .getReturnEdgeFunction(n, sCalledProcN, eP, d4,
                                                 retSiteN, d5);
                  // add ret PDS rule
                  auto d4_k = wali::getKey(d4);
                  DKey[d4] = d4_k;
                  auto d5_k = wali::getKey(d5);
                  DKey[d5] = d5_k;
                  wali::ref_ptr<JoinLatticeToSemiRingElem<L>> wptrRet(
                      new JoinLatticeToSemiRingElem<L>(
                          f5, static_cast<JoinLattice<L> &>(Problem)));
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "ADD RET RULE (CALL): "
                                << Problem.DtoString(d4) << ", "
                                << Problem.NtoString(retSiteN) << ", "
                                << Problem.DtoString(d5) << ", " << *wptrRet);
                  std::set<N> exitPointsN =
                      IDESolver<N, D, F, T, V, L, I>::ICF->getExitPointsOf(
                          IDESolver<N, D, F, T, V, L, I>::ICF->getFunctionOf(
                              sP));
                  for (auto exitPointN : exitPointsN) {
                    auto exitPointN_k = wali::getKey(exitPointN);
                    PDS->add_rule(d4_k, exitPointN_k, d5_k, wptrRet);
                  }
                  if (!SRElem.is_valid()) {
                    SRElem = wptrRet;
                  }
                  INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
                  // compose call * calleeSummary * return edge functions
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "Compose: " << f5->str() << " * "
                                << fCalleeSummary->str() << " * " << f4->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "         (return * calleeSummary * call)");
                  std::shared_ptr<EdgeFunction<L>> fPrime =
                      f4->composeWith(fCalleeSummary)->composeWith(f5);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "       = " << fPrime->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
                  D d5_restoredCtx =
                      IDESolver<N, D, F, T, V, L,
                                I>::restoreContextOnReturnedFact(n, d2, d5);
                  // propagte the effects of the entire call
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "Compose: " << fPrime->str() << " * "
                                << f->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
                  IDESolver<N, D, F, T, V, L, I>::propagate(
                      d1, retSiteN, d5_restoredCtx, f->composeWith(fPrime), n,
                      false);
                }
              }
            }
          }
        }
      }
      // line 17-19 of Naeem/Lhotak/Rodriguez
      // process intra-procedural flows along call-to-return flow functions
      for (N returnSiteN : returnSiteNs) {
        std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction =
            IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                .getCallToRetFlowFunction(n, returnSiteN, callees);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<D> returnFacts =
            IDESolver<N, D, F, T, V, L, I>::computeCallToReturnFlowFunction(
                callToReturnFlowFunction, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", returnFacts.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        IDESolver<N, D, F, T, V, L, I>::saveEdges(n, returnSiteN, d2,
                                                  returnFacts, false);
        for (D d3 : returnFacts) {
          std::shared_ptr<EdgeFunction<L>> edgeFnE =
              IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                  .getCallToRetEdgeFunction(n, d2, returnSiteN, d3, callees);
          // add calltoret PDS rule
          auto d2_k = wali::getKey(d2);
          DKey[d2] = d2_k;
          auto d3_k = wali::getKey(d3);
          DKey[d3] = d3_k;
          auto n_k = wali::getKey(n);
          auto returnSiteN_k = wali::getKey(returnSiteN);
          wali::ref_ptr<JoinLatticeToSemiRingElem<L>> wptr(
              new JoinLatticeToSemiRingElem<L>(
                  edgeFnE, static_cast<JoinLattice<L> &>(Problem)));
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "ADD CALLTORET RULE: " << Problem.DtoString(d2)
                        << " | " << Problem.NtoString(n) << " --> "
                        << Problem.DtoString(d3) << ", "
                        << Problem.NtoString(returnSiteN) << ", " << *wptr);
          PDS->add_rule(d2_k, n_k, d3_k, returnSiteN_k, wptr);
          if (!SRElem.is_valid()) {
            SRElem = wptr;
          }
          INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Compose: " << edgeFnE->str() << " * " << f->str());
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
          IDESolver<N, D, F, T, V, L, I>::propagate(
              d1, returnSiteN, d3, f->composeWith(edgeFnE), n, false);
        }
      }
    }
  }

  void processExit(PathEdge<N, D> edge) override {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "WPDS::processExit");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Exit", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process exit at target: "
                  << this->ideTabulationProblem.NtoString(edge.getTarget()));
    N n = edge.getTarget(); // an exit node; line 21...
    std::shared_ptr<EdgeFunction<L>> f =
        IDESolver<N, D, F, T, V, L, I>::jumpFunction(edge);
    F functionThatNeedsSummary =
        IDESolver<N, D, F, T, V, L, I>::ICF->getFunctionOf(n);
    D d1 = edge.factAtSource();
    D d2 = edge.factAtTarget();
    // for each of the method's start points, determine incoming calls
    std::set<N> startPointsOf =
        IDESolver<N, D, F, T, V, L, I>::ICF->getStartPointsOf(
            functionThatNeedsSummary);
    std::map<N, std::set<D>> inc;
    for (N sP : startPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      IDESolver<N, D, F, T, V, L, I>::addEndSummary(sP, d1, n, d2, f);
      for (auto entry : IDESolver<N, D, F, T, V, L, I>::incoming(d1, sP)) {
        inc[entry.first] = std::set<D>{entry.second};
      }
    }
    IDESolver<N, D, F, T, V, L, I>::printEndSummaryTab();
    IDESolver<N, D, F, T, V, L, I>::printIncomingTab();
    // for each incoming call edge already processed
    //(see processCall(..))
    for (auto entry : inc) {
      // line 22
      N c = entry.first;
      // for each return site
      for (N retSiteC :
           IDESolver<N, D, F, T, V, L, I>::ICF->getReturnSitesOfCallAt(c)) {
        // compute return-flow function
        std::shared_ptr<FlowFunction<D>> retFunction =
            IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                .getRetFlowFunction(c, functionThatNeedsSummary, n, retSiteC);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        // for each incoming-call value
        for (D d4 : entry.second) {
          std::set<D> targets =
              IDESolver<N, D, F, T, V, L, I>::computeReturnFlowFunction(
                  retFunction, d1, d2, c, entry.second);
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<N, D, F, T, V, L, I>::saveEdges(n, retSiteC, d2, targets,
                                                    true);
          std::cout << "RETURN TARGETS: " << targets.size() << std::endl;
          // for each target value at the return site
          // line 23
          for (D d5 : targets) {
            // compute composed function
            // get call edge function
            std::shared_ptr<EdgeFunction<L>> f4 =
                IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                    .getCallEdgeFunction(
                        c, d4,
                        IDESolver<N, D, F, T, V, L, I>::ICF->getFunctionOf(n),
                        d1);
            // get return edge function
            std::shared_ptr<EdgeFunction<L>> f5 =
                IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                    .getReturnEdgeFunction(
                        c,
                        IDESolver<N, D, F, T, V, L, I>::ICF->getFunctionOf(n),
                        n, d2, retSiteC, d5);
            // add ret PDS rule
            auto d1_k = wali::getKey(d1);
            DKey[d1] = d1_k;
            auto d2_k = wali::getKey(d2);
            DKey[d2] = d2_k;
            auto d5_k = wali::getKey(d5);
            DKey[d5] = d5_k;
            auto n_k = wali::getKey(n);
            wali::ref_ptr<JoinLatticeToSemiRingElem<L>> wptr(
                new JoinLatticeToSemiRingElem<L>(
                    f5, static_cast<JoinLattice<L> &>(Problem)));
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "ADD RET RULE: " << Problem.DtoString(d2) << ", "
                          << Problem.NtoString(n) << ", "
                          << Problem.DtoString(d5) << ", " << *wptr);
            PDS->add_rule(d2_k, n_k, d5_k, wptr);
            if (!SRElem.is_valid()) {
              SRElem = wptr;
            }
            INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
            // compose call function * function * return function
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << f5->str() << " * " << f->str()
                          << " * " << f4->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "         (return * function * call)");
            std::shared_ptr<EdgeFunction<L>> fPrime =
                f4->composeWith(f)->composeWith(f5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "       = " << fPrime->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
            // for each jump function coming into the call, propagate to return
            // site using the composed function
            for (auto valAndFunc :
                 IDESolver<N, D, F, T, V, L, I>::jumpFn->reverseLookup(c, d4)) {
              std::shared_ptr<EdgeFunction<L>> f3 = valAndFunc.second;
              if (!f3->equal_to(IDESolver<N, D, F, T, V, L, I>::allTop)) {
                D d3 = valAndFunc.first;
                D d5_restoredCtx =
                    IDESolver<N, D, F, T, V, L,
                              I>::restoreContextOnReturnedFact(c, d4, d5);
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                              << "Compose: " << fPrime->str() << " * "
                              << f3->str());
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
                IDESolver<N, D, F, T, V, L, I>::propagate(
                    d3, retSiteC, d5_restoredCtx, f3->composeWith(fPrime), c,
                    false);
              }
            }
          }
        }
      }
    }
    // handling for unbalanced problems where we return out of a method with a
    // fact for which we have no incoming flow.
    // note: we propagate that way only values that originate from ZERO, as
    // conditionally generated values should only
    // be propagated into callers that have an incoming edge for this condition
    if (IDESolver<N, D, F, T, V, L, I>::SolverConfig.followReturnsPastSeeds &&
        inc.empty() &&
        IDESolver<N, D, F, T, V, L, I>::ideTabulationProblem.isZeroValue(d1)) {
      std::set<N> callers = IDESolver<N, D, F, T, V, L, I>::ICF->getCallersOf(
          functionThatNeedsSummary);
      for (N c : callers) {
        for (N retSiteC :
             IDESolver<N, D, F, T, V, L, I>::ICF->getReturnSitesOfCallAt(c)) {
          std::shared_ptr<FlowFunction<D>> retFunction =
              IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                  .getRetFlowFunction(c, functionThatNeedsSummary, n, retSiteC);
          INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          std::set<D> targets =
              IDESolver<N, D, F, T, V, L, I>::computeReturnFlowFunction(
                  retFunction, d1, d2, c,
                  std::set<D>{IDESolver<N, D, F, T, V, L, I>::zeroValue});
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<N, D, F, T, V, L, I>::saveEdges(n, retSiteC, d2, targets,
                                                    true);
          for (D d5 : targets) {
            std::shared_ptr<EdgeFunction<L>> f5 =
                IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                    .getReturnEdgeFunction(
                        c,
                        IDESolver<N, D, F, T, V, L, I>::ICF->getFunctionOf(n),
                        n, d2, retSiteC, d5);
            INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << f5->str() << " * " << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
            IDESolver<N, D, F, T, V, L, I>::propagteUnbalancedReturnFlow(
                retSiteC, d5, f->composeWith(f5), c);
            // register for value processing (2nd IDE phase)
            IDESolver<N, D, F, T, V, L, I>::unbalancedRetSites.insert(retSiteC);
          }
        }
      }
      // in cases where there are no callers, the return statement would
      // normally not be processed at all; this might be undesirable if
      // the flow function has a side effect such as registering a taint;
      // instead we thus call the return flow function will a null caller
      if (callers.empty()) {
        std::shared_ptr<FlowFunction<D>> retFunction =
            IDESolver<N, D, F, T, V, L, I>::cachedFlowEdgeFunctions
                .getRetFlowFunction(nullptr, functionThatNeedsSummary, n,
                                    nullptr);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        retFunction->computeTargets(d2);
      }
    }
  }

  void doForwardSearch(wali::wfa::WFA &Answer) {
    // Create an automaton to AcceptingState the configuration:
    // <ZeroPDSState, entry>
    for (auto seed : IDESolver<N, D, F, T, V, L, I>::initialSeeds) {
      wali::Key entry = wali::getKey(seed.first);
      Query.addTrans(ZeroPDSState, entry, AcceptingState, SRElem->one());
    }
    Query.set_initial_state(ZeroPDSState);
    Query.add_final_state(AcceptingState);
    Query.print(std::cout << "BEFORE POSTSTAR\n");
    std::ofstream before("before_poststar.dot");
    Query.print_dot(before, true);
    PDS->poststar(Query, Answer);
    Answer.print(std::cout << "AFTER POSTSTAR\n");
    std::ofstream after("after_poststar.dot");
    Answer.print_dot(after, true);
  }

  void doBackwardSearch(wali::Key node, wali::wfa::WFA &Answer) {
    // Create an automaton to AcceptingState the configurations {n \Gamma^*}
    // Find the set of all return points
    wali::wpds::WpdsStackSymbols syms;
    PDS->for_each(syms);
    auto alloca1 = &IDESolver<N, D, F, T, V, L, I>::ICF->getFunction("main")
                        ->front()
                        .front();
    auto a1key = wali::getKey(alloca1);
    // the weight is essential here!
    Query.addTrans(a1key, node, AcceptingState, SRElem->one());
    std::set<wali::Key>::iterator it;
    for (it = syms.returnPoints.begin(); it != syms.returnPoints.end(); it++) {
      wali::getKeySource(*it)->print(std::cout);
      std::cout << std::endl;
      Query.addTrans(AcceptingState, *it, AcceptingState, SRElem->one());
    }
    Query.set_initial_state(a1key);
    Query.add_final_state(AcceptingState);
    Query.print(std::cout << "BEFORE PRESTAR\n");
    std::ofstream before("before_prestar.dot");
    Query.print_dot(before, true);
    PDS->prestar(Query, Answer);
    Answer.print(std::cout << "AFTER PRESTAR\n");
    std::ofstream after("after_prestar.dot");
    Answer.print_dot(after, true);
  }

  void doBackwardSearch(std::vector<wali::Key> node_stack,
                        wali::wfa::WFA &Answer) {
    assert(!node_stack.empty());
    // Create an automaton to AcceptingState the configuration <ZeroPDSState,
    // node_stack>
    wali::Key temp_from = ZeroPDSState;
    wali::Key temp_to = wali::WALI_EPSILON;
    for (size_t i = 0; i < node_stack.size(); i++) {
      std::stringstream ss;
      ss << "__tmp_state_" << i;
      temp_to = wali::getKey(ss.str());
      Query.addTrans(temp_from, node_stack[i], temp_to, SRElem->one());
      temp_from = temp_to;
    }
    Query.set_initial_state(ZeroPDSState);
    Query.add_final_state(temp_to);
    PDS->prestar(Query, Answer);
  }

  std::unordered_map<D, L> resultsAt(N stmt, bool stripZero = false) override {
    std::unordered_map<D, L> Results;
    wali::wfa::Trans goal;
    for (auto Entry : DKey) {
      // Method 1: If query 'stmt' is located within the same function as the
      // starting point
      if (Answer.find(Entry.second, wali::getKey(stmt), AcceptingState, goal)) {
        Results.insert(std::make_pair(
            Entry.first,
            static_cast<JoinLatticeToSemiRingElem<L> &>(*goal.weight())
                .F->computeTarget(L{})));
      } else {
        // Method 2: If query 'stmt' is located in a different function
        wali::sem_elem_t ret = SRElem->zero();
        wali::wfa::TransSet tset;
        wali::wfa::TransSet::iterator titer;
        tset = Answer.match(Entry.second, wali::getKey(stmt));
        for (titer = tset.begin(); titer != tset.end(); titer++) {
          wali::wfa::ITrans *t = *titer;
          wali::sem_elem_t tmp(Answer.getState(t->to())->weight());
          tmp = tmp->extend(t->weight());
          ret = ret->combine(tmp);
        }
        if (!ret->equal(SRElem->zero())) {
          Results.insert(std::make_pair(
              Entry.first, static_cast<JoinLatticeToSemiRingElem<L> &>(*ret)
                               .F->computeTarget(L{})));
        }
      }
    }
    if (stripZero) {
      Results.erase(ZeroValue);
    }
    return Results;
  }

  L resultAt(N stmt, D fact) override {
    wali::wfa::Trans goal;
    if (Answer.find(wali::getKey(fact), wali::getKey(stmt), AcceptingState,
                    goal)) {
      return static_cast<JoinLatticeToSemiRingElem<L> &>(*goal.weight())
          .F->computeTarget(L{});
    }
    wali::sem_elem_t ret = SRElem->zero();
    wali::wfa::TransSet tset;
    wali::wfa::TransSet::iterator titer;
    tset = Answer.match(wali::getKey(fact), wali::getKey(stmt));
    for (titer = tset.begin(); titer != tset.end(); titer++) {
      wali::wfa::ITrans *t = *titer;
      wali::sem_elem_t tmp(Answer.getState(t->to())->weight());
      tmp = tmp->extend(t->weight());
      ret = ret->combine(tmp);
    }
    if (!ret->equal(SRElem->zero())) {
      return static_cast<JoinLatticeToSemiRingElem<L> &>(*ret).F->computeTarget(
          L{});
    }
    throw std::runtime_error("Requested invalid fact!");
  }
};

} // namespace psr

#endif
