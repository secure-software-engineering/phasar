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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
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
} // namespace llvm

namespace psr {

template <typename AnalysisDomainTy>
class WPDSSolver : public IDESolver<AnalysisDomainTy> {
public:
  using ProblemTy = IDETabulationProblem<AnalysisDomainTy>;
  using container_type = typename ProblemTy::container_type;
  using FlowFunctionPtrType = typename ProblemTy::FlowFunctionPtrType;
  using EdgeFunctionPtrType = typename ProblemTy::EdgeFunctionPtrType;

  using l_t = typename AnalysisDomainTy::l_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;

private:
  WPDSProblem<AnalysisDomainTy> &Problem;
  WPDSSolverConfig SolverConf;
  std::vector<n_t> Stack;
  std::unique_ptr<wali::wpds::WPDS> PDS;
  d_t ZeroValue;
  wali::Key ZeroPDSState;
  wali::Key AcceptingState;
  std::unordered_map<d_t, wali::Key> DKey;
  wali::wfa::WFA Query;
  wali::wfa::WFA Answer;
  wali::sem_elem_t SRElem;
  Table<n_t, d_t, std::map<n_t, std::set<d_t>>> incomingtab;

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
  WPDSSolver(WPDSProblem<AnalysisDomainTy> &Problem)
      : IDESolver<AnalysisDomainTy>(Problem), Problem(Problem),
        SolverConf(Problem.getWPDSSolverConfig()),
        PDS(makePDS(SolverConf.wpdsty, SolverConf.recordWitnesses)),
        ZeroValue(Problem.getZeroValue()),
        AcceptingState(wali::getKey("__accept")), SRElem(nullptr) {
    ZeroPDSState = wali::getKey(ZeroValue);
    DKey[ZeroValue] = ZeroPDSState;
  }
  ~WPDSSolver() override = default;

  void solve() override {

    // Construct the PDS
    IDESolver<AnalysisDomainTy>::submitInitalSeeds();
    std::ofstream pdsfile("pds.dot");
    PDS->print_dot(pdsfile, true);
    pdsfile.flush();
    pdsfile.close();
    // test the SRElem
    wali::test_semelem_impl(SRElem);
    // Solve the PDS
    wali::sem_elem_t ret = nullptr;
    if (WPDSSearchDirection::FORWARD == SolverConf.searchDirection) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "FORWARD");
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
          wali::getKey(&IDESolver<AnalysisDomainTy>::ICF->getFunction("main")
                            ->back()
                            .back());
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "BACKWARD");
      doBackwardSearch(retnode, Answer);
      Answer.path_summary();

      // access the results
      // wali::wfa::Trans goal;
      // // check all data-flow facts
      // std::cout << "All d_t(s)" << std::endl;
      // for (auto Entry : DKey) {
      //   if (Answer.find(Entry.second, retnode, AcceptingState, goal)) {
      //     std::cout << "FOUND ANSWER!" << std::endl;
      //     std::cout << llvmIRToString(Entry.first);
      //     goal.weight()->print(std::cout << " :--- weight ---: ");
      //     std::cout << " : --- "
      //               << static_cast<JoinLatticeToSemiRingElem<l_t>
      //               &>(*goal.weight())
      //                      .F->computeTarget(l_t{});
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
      //           << static_cast<JoinLatticeToSemiRingElem<l_t>
      //           &>(*ret).F->computeTarget(l_t{});
      // std::cout << std::endl;
    }
  }

  void processNormalFlow(PathEdge<n_t, d_t> edge) override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "WPDS::processNormal");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Normal", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process normal at target: "
                  << this->ideTabulationProblem.NtoString(edge.getTarget()));
    d_t d1 = edge.factAtSource();
    n_t n = edge.getTarget();
    d_t d2 = edge.factAtTarget();
    EdgeFunctionPtrType f = IDESolver<AnalysisDomainTy>::jumpFunction(edge);
    auto successorInst = IDESolver<AnalysisDomainTy>::ICF->getSuccsOf(n);
    for (auto f : successorInst) {
      FlowFunctionPtrType flowFunction =
          IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
              .getNormalFlowFunction(n, f);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      std::set<d_t> res =
          IDESolver<AnalysisDomainTy>::computeNormalFlowFunction(flowFunction,
                                                                 d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      IDESolver<AnalysisDomainTy>::saveEdges(n, f, d2, res, false);
      for (d_t d3 : res) {
        EdgeFunctionPtrType g =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getNormalEdgeFunction(n, d2, f, d3);
        // add normal PDS rule
        auto d2_k = wali::getKey(d2);
        DKey[d2] = d2_k;
        auto d3_k = wali::getKey(d3);
        DKey[d3] = d3_k;
        auto n_k = wali::getKey(n);
        auto f_k = wali::getKey(f);
        wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> wptr;
        wptr = new JoinLatticeToSemiRingElem<l_t>(
            g, static_cast<JoinLattice<l_t> &>(Problem));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "ADD NORMAL RULE: " << Problem.DtoString(d2) << " | "
                      << Problem.NtoString(n) << " --> "
                      << Problem.DtoString(d3) << " | " << Problem.DtoString(f)
                      << ", " << *wptr << ")");
        PDS->add_rule(d2_k, n_k, d3_k, f_k, wptr);
        if (!SRElem.is_valid()) {
          SRElem = wptr;
        }
        EdgeFunctionPtrType fprime = f->composeWith(g);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Compose: " << g->str() << " * " << f->str());
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        IDESolver<AnalysisDomainTy>::propagate(d1, f, d3, fprime, nullptr,
                                               false);
      }
    }
  }

  void processCall(PathEdge<n_t, d_t> edge) override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "WPDS::processCall");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Call", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process call at target: "
                  << this->ideTabulationProblem.NtoString(edge.getTarget()));
    d_t d1 = edge.factAtSource();
    n_t n = edge.getTarget(); // a call node; line 14...
    d_t d2 = edge.factAtTarget();
    EdgeFunctionPtrType f = IDESolver<AnalysisDomainTy>::jumpFunction(edge);
    std::set<n_t> returnSiteNs =
        IDESolver<AnalysisDomainTy>::ICF->getReturnSitesOfCallAt(n);
    std::set<f_t> callees =
        IDESolver<AnalysisDomainTy>::ICF->getCalleesOfCallAt(n);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "Possible callees:");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "  " << callee->getName().str());
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "Possible return sites:");
    for (auto ret : returnSiteNs) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "  " << this->ideTabulationProblem.NtoString(ret));
    }
    // for each possible callee
    for (f_t sCalledProcN : callees) { // still line 14
      // check if a special summary for the called procedure exists
      FlowFunctionPtrType specialSum =
          IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
              .getSummaryFlowFunction(n, sCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (specialSum) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Found and process special summary");
        for (n_t returnSiteN : returnSiteNs) {
          std::set<d_t> res =
              IDESolver<AnalysisDomainTy>::computeSummaryFlowFunction(
                  specialSum, d1, d2);
          INC_COUNTER("SpecialSummary-FF Application", 1,
                      PAMM_SEVERITY_LEVEL::Full);
          ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<AnalysisDomainTy>::saveEdges(n, returnSiteN, d2, res,
                                                 false);
          for (d_t d3 : res) {
            EdgeFunctionPtrType sumEdgFnE =
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getSummaryEdgeFunction(n, d2, returnSiteN, d3);
            INC_COUNTER("SpecialSummary-EF Queries", 1,
                        PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Compose: " << sumEdgFnE->str() << " * "
                          << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
            IDESolver<AnalysisDomainTy>::propagate(
                d1, returnSiteN, d3, f->composeWith(sumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        FlowFunctionPtrType function =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getCallFlowFunction(n, sCalledProcN);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<d_t> res =
            IDESolver<AnalysisDomainTy>::computeCallFlowFunction(function, d1,
                                                                 d2);
        ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        // for each callee's start point(s)
        std::set<n_t> startPointsOf =
            IDESolver<AnalysisDomainTy>::ICF->getStartPointsOf(sCalledProcN);
        if (startPointsOf.empty()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Start points of '" +
                               this->ICF->getFunctionName(sCalledProcN) +
                               "' currently not available!");
        }
        // if startPointsOf is empty, the called function is a declaration
        for (n_t sP : startPointsOf) {
          IDESolver<AnalysisDomainTy>::saveEdges(n, sP, d2, res, true);
          // for each result node of the call-flow function
          for (d_t d3 : res) {
            // create initial self-loop
            IDESolver<AnalysisDomainTy>::propagate(
                d3, sP, d3, EdgeIdentity<l_t>::getInstance(), n,
                false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            IDESolver<AnalysisDomainTy>::addIncoming(sP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            std::set<typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell>
                endSumm = std::set<
                    typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell>(
                    IDESolver<AnalysisDomainTy>::endSummary(sP, d3));
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
            for (typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell entry :
                 endSumm) {
              n_t eP = entry.getRowKey();
              d_t d4 = entry.getColumnKey();
              EdgeFunctionPtrType fCalleeSummary = entry.getValue();
              // for each return site
              for (n_t retSiteN : returnSiteNs) {
                // compute return-flow function
                FlowFunctionPtrType retFunction =
                    IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                        .getRetFlowFunction(n, sCalledProcN, eP, retSiteN);
                INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
                std::set<d_t> returnedFacts =
                    IDESolver<AnalysisDomainTy>::computeReturnFlowFunction(
                        retFunction, d3, d4, n, std::set<d_t>{d2});
                ADD_TO_HISTOGRAM("Data-flow facts", returnedFacts.size(), 1,
                                 PAMM_SEVERITY_LEVEL::Full);
                IDESolver<AnalysisDomainTy>::saveEdges(eP, retSiteN, d4,
                                                       returnedFacts, true);
                // for each target value of the function
                for (d_t d5 : returnedFacts) {
                  // update the caller-side summary function
                  // get call edge function
                  EdgeFunctionPtrType f4 =
                      IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                          .getCallEdgeFunction(n, d2, sCalledProcN, d3);
                  // add call PDS rule
                  auto d2_k = wali::getKey(d2);
                  DKey[d2] = d2_k;
                  auto d3_k = wali::getKey(d3);
                  DKey[d3] = d3_k;
                  auto n_k = wali::getKey(n);
                  auto sP_k = wali::getKey(sP);
                  wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> wptrCall(
                      new JoinLatticeToSemiRingElem<l_t>(
                          f4, static_cast<JoinLattice<l_t> &>(Problem)));
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
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
                  EdgeFunctionPtrType f5 =
                      IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                          .getReturnEdgeFunction(n, sCalledProcN, eP, d4,
                                                 retSiteN, d5);
                  // add ret PDS rule
                  auto d4_k = wali::getKey(d4);
                  DKey[d4] = d4_k;
                  auto d5_k = wali::getKey(d5);
                  DKey[d5] = d5_k;
                  wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> wptrRet(
                      new JoinLatticeToSemiRingElem<l_t>(
                          f5, static_cast<JoinLattice<l_t> &>(Problem)));
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "ADD RET RULE (CALL): "
                                << Problem.DtoString(d4) << ", "
                                << Problem.NtoString(retSiteN) << ", "
                                << Problem.DtoString(d5) << ", " << *wptrRet);
                  std::set<n_t> exitPointsN =
                      IDESolver<AnalysisDomainTy>::ICF->getExitPointsOf(
                          IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(sP));
                  for (auto exitPointN : exitPointsN) {
                    auto exitPointN_k = wali::getKey(exitPointN);
                    PDS->add_rule(d4_k, exitPointN_k, d5_k, wptrRet);
                  }
                  if (!SRElem.is_valid()) {
                    SRElem = wptrRet;
                  }
                  INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
                  // compose call * calleeSummary * return edge functions
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "Compose: " << f5->str() << " * "
                                << fCalleeSummary->str() << " * " << f4->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "         (return * calleeSummary * call)");
                  EdgeFunctionPtrType fPrime =
                      f4->composeWith(fCalleeSummary)->composeWith(f5);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "       = " << fPrime->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
                  d_t d5_restoredCtx =
                      IDESolver<AnalysisDomainTy>::restoreContextOnReturnedFact(
                          n, d2, d5);
                  // propagte the effects of the entire call
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "Compose: " << fPrime->str() << " * "
                                << f->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
                  IDESolver<AnalysisDomainTy>::propagate(
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
      for (n_t returnSiteN : returnSiteNs) {
        FlowFunctionPtrType callToReturnFlowFunction =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getCallToRetFlowFunction(n, returnSiteN, callees);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<d_t> returnFacts =
            IDESolver<AnalysisDomainTy>::computeCallToReturnFlowFunction(
                callToReturnFlowFunction, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", returnFacts.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        IDESolver<AnalysisDomainTy>::saveEdges(n, returnSiteN, d2, returnFacts,
                                               false);
        for (d_t d3 : returnFacts) {
          EdgeFunctionPtrType edgeFnE =
              IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                  .getCallToRetEdgeFunction(n, d2, returnSiteN, d3, callees);
          // add calltoret PDS rule
          auto d2_k = wali::getKey(d2);
          DKey[d2] = d2_k;
          auto d3_k = wali::getKey(d3);
          DKey[d3] = d3_k;
          auto n_k = wali::getKey(n);
          auto returnSiteN_k = wali::getKey(returnSiteN);
          wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> wptr(
              new JoinLatticeToSemiRingElem<l_t>(
                  edgeFnE, static_cast<JoinLattice<l_t> &>(Problem)));
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "ADD CALLTORET RULE: " << Problem.DtoString(d2)
                        << " | " << Problem.NtoString(n) << " --> "
                        << Problem.DtoString(d3) << ", "
                        << Problem.NtoString(returnSiteN) << ", " << *wptr);
          PDS->add_rule(d2_k, n_k, d3_k, returnSiteN_k, wptr);
          if (!SRElem.is_valid()) {
            SRElem = wptr;
          }
          INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Compose: " << edgeFnE->str() << " * " << f->str());
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
          IDESolver<AnalysisDomainTy>::propagate(
              d1, returnSiteN, d3, f->composeWith(edgeFnE), n, false);
        }
      }
    }
  }

  void processExit(PathEdge<n_t, d_t> edge) override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "WPDS::processExit");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Exit", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process exit at target: "
                  << this->ideTabulationProblem.NtoString(edge.getTarget()));
    n_t n = edge.getTarget(); // an exit node; line 21...
    EdgeFunctionPtrType f = IDESolver<AnalysisDomainTy>::jumpFunction(edge);
    f_t functionThatNeedsSummary =
        IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n);
    d_t d1 = edge.factAtSource();
    d_t d2 = edge.factAtTarget();
    // for each of the method's start points, determine incoming calls
    std::set<n_t> startPointsOf =
        IDESolver<AnalysisDomainTy>::ICF->getStartPointsOf(
            functionThatNeedsSummary);
    std::map<n_t, std::set<d_t>> inc;
    for (n_t sP : startPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      IDESolver<AnalysisDomainTy>::addEndSummary(sP, d1, n, d2, f);
      for (auto entry : IDESolver<AnalysisDomainTy>::incoming(d1, sP)) {
        inc[entry.first] = std::set<d_t>{entry.second};
      }
    }
    IDESolver<AnalysisDomainTy>::printEndSummaryTab();
    IDESolver<AnalysisDomainTy>::printIncomingTab();
    // for each incoming call edge already processed
    //(see processCall(..))
    for (auto entry : inc) {
      // line 22
      n_t c = entry.first;
      // for each return site
      for (n_t retSiteC :
           IDESolver<AnalysisDomainTy>::ICF->getReturnSitesOfCallAt(c)) {
        // compute return-flow function
        FlowFunctionPtrType retFunction =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getRetFlowFunction(c, functionThatNeedsSummary, n, retSiteC);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        // for each incoming-call value
        for (d_t d4 : entry.second) {
          std::set<d_t> targets =
              IDESolver<AnalysisDomainTy>::computeReturnFlowFunction(
                  retFunction, d1, d2, c, entry.second);
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<AnalysisDomainTy>::saveEdges(n, retSiteC, d2, targets,
                                                 true);
          std::cout << "RETURN TARGETS: " << targets.size() << std::endl;
          // for each target value at the return site
          // line 23
          for (d_t d5 : targets) {
            // compute composed function
            // get call edge function
            EdgeFunctionPtrType f4 =
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getCallEdgeFunction(
                        c, d4,
                        IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n), d1);
            // get return edge function
            EdgeFunctionPtrType f5 =
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getReturnEdgeFunction(
                        c, IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n),
                        n, d2, retSiteC, d5);
            // add ret PDS rule
            auto d1_k = wali::getKey(d1);
            DKey[d1] = d1_k;
            auto d2_k = wali::getKey(d2);
            DKey[d2] = d2_k;
            auto d5_k = wali::getKey(d5);
            DKey[d5] = d5_k;
            auto n_k = wali::getKey(n);
            wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> wptr(
                new JoinLatticeToSemiRingElem<l_t>(
                    f5, static_cast<JoinLattice<l_t> &>(Problem)));
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "ADD RET RULE: " << Problem.DtoString(d2) << ", "
                          << Problem.NtoString(n) << ", "
                          << Problem.DtoString(d5) << ", " << *wptr);
            PDS->add_rule(d2_k, n_k, d5_k, wptr);
            if (!SRElem.is_valid()) {
              SRElem = wptr;
            }
            INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
            // compose call function * function * return function
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Compose: " << f5->str() << " * " << f->str()
                          << " * " << f4->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "         (return * function * call)");
            EdgeFunctionPtrType fPrime = f4->composeWith(f)->composeWith(f5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "       = " << fPrime->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
            // for each jump function coming into the call, propagate to return
            // site using the composed function
            for (auto valAndFunc :
                 IDESolver<AnalysisDomainTy>::jumpFn->reverseLookup(c, d4)) {
              EdgeFunctionPtrType f3 = valAndFunc.second;
              if (!f3->equal_to(IDESolver<AnalysisDomainTy>::allTop)) {
                d_t d3 = valAndFunc.first;
                d_t d5_restoredCtx =
                    IDESolver<AnalysisDomainTy>::restoreContextOnReturnedFact(
                        c, d4, d5);
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                              << "Compose: " << fPrime->str() << " * "
                              << f3->str());
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
                IDESolver<AnalysisDomainTy>::propagate(
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
    if (IDESolver<AnalysisDomainTy>::SolverConfig.followReturnsPastSeeds &&
        inc.empty() &&
        IDESolver<AnalysisDomainTy>::ideTabulationProblem.isZeroValue(d1)) {
      std::set<n_t> callers = IDESolver<AnalysisDomainTy>::ICF->getCallersOf(
          functionThatNeedsSummary);
      for (n_t c : callers) {
        for (n_t retSiteC :
             IDESolver<AnalysisDomainTy>::ICF->getReturnSitesOfCallAt(c)) {
          FlowFunctionPtrType retFunction =
              IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                  .getRetFlowFunction(c, functionThatNeedsSummary, n, retSiteC);
          INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          std::set<d_t> targets =
              IDESolver<AnalysisDomainTy>::computeReturnFlowFunction(
                  retFunction, d1, d2, c,
                  std::set<d_t>{IDESolver<AnalysisDomainTy>::zeroValue});
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<AnalysisDomainTy>::saveEdges(n, retSiteC, d2, targets,
                                                 true);
          for (d_t d5 : targets) {
            EdgeFunctionPtrType f5 =
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getReturnEdgeFunction(
                        c, IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n),
                        n, d2, retSiteC, d5);
            INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Compose: " << f5->str() << " * " << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
            IDESolver<AnalysisDomainTy>::propagteUnbalancedReturnFlow(
                retSiteC, d5, f->composeWith(f5), c);
            // register for value processing (2nd IDE phase)
            IDESolver<AnalysisDomainTy>::unbalancedRetSites.insert(retSiteC);
          }
        }
      }
      // in cases where there are no callers, the return statement would
      // normally not be processed at all; this might be undesirable if
      // the flow function has a side effect such as registering a taint;
      // instead we thus call the return flow function will a null caller
      if (callers.empty()) {
        FlowFunctionPtrType retFunction =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
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
    for (auto seed : IDESolver<AnalysisDomainTy>::initialSeeds) {
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
    auto alloca1 =
        &IDESolver<AnalysisDomainTy>::ICF->getFunction("main")->front().front();
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

  std::unordered_map<d_t, l_t> resultsAt(n_t stmt,
                                         bool stripZero = false) override {
    std::unordered_map<d_t, l_t> Results;
    wali::wfa::Trans goal;
    for (auto Entry : DKey) {
      // Method 1: If query 'stmt' is located within the same function as the
      // starting point
      if (Answer.find(Entry.second, wali::getKey(stmt), AcceptingState, goal)) {
        Results.insert(std::make_pair(
            Entry.first,
            static_cast<JoinLatticeToSemiRingElem<l_t> &>(*goal.weight())
                .F->computeTarget(l_t{})));
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
              Entry.first, static_cast<JoinLatticeToSemiRingElem<l_t> &>(*ret)
                               .F->computeTarget(l_t{})));
        }
      }
    }
    if (stripZero) {
      Results.erase(ZeroValue);
    }
    return Results;
  }

  l_t resultAt(n_t stmt, d_t fact) override {
    wali::wfa::Trans goal;
    if (Answer.find(wali::getKey(fact), wali::getKey(stmt), AcceptingState,
                    goal)) {
      return static_cast<JoinLatticeToSemiRingElem<l_t> &>(*goal.weight())
          .F->computeTarget(l_t{});
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
      return static_cast<JoinLatticeToSemiRingElem<l_t> &>(*ret)
          .F->computeTarget(l_t{});
    }
    throw std::runtime_error("Requested invalid fact!");
  }
};

} // namespace psr

#endif
