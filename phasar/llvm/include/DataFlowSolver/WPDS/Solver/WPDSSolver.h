/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_SOLVER_WPDSSOLVER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_SOLVER_WPDSSOLVER_H

#include <fstream>
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
  Table<n_t, d_t, std::map<n_t, std::set<d_t>>> Incomingtab;

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
        PDS(makePDS(SolverConf.WPDSType, SolverConf.RecordWitnesses)),
        ZeroValue(Problem.getZeroValue()),
        AcceptingState(wali::getKey("__accept")), SRElem(nullptr),
        ZeroPDSState(wali::getKey(ZeroValue)) {
    DKey[ZeroValue] = ZeroPDSState;
  }
  ~WPDSSolver() override = default;

  void solve() override {

    // Construct the PDS
    IDESolver<AnalysisDomainTy>::submitInitalSeeds();
    std::ofstream PDSFile("pds.dot");
    PDS->print_dot(PDSFile, true);
    PDSFile.flush();
    PDSFile.close();
    // test the SRElem
    wali::test_semelem_impl(SRElem);
    // Solve the PDS
    wali::sem_elem_t Ret = nullptr;
    if (WPDSSearchDirection::FORWARD == SolverConf.Direction) {
      PHASAR_LOG_LEVEL(DEBUG, "FORWARD");
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
      auto RetNode =
          wali::getKey(&IDESolver<AnalysisDomainTy>::ICF->getFunction("main")
                            ->back()
                            .back());
      PHASAR_LOG_LEVEL(DEBUG, "BACKWARD");
      doBackwardSearch(RetNode, Answer);
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

  void processNormalFlow(PathEdge<n_t, d_t> Edge) override {
    PHASAR_LOG_LEVEL(DEBUG, "WPDS::processNormal");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Normal", 1, PAMM_SEVERITY_LEVEL::Full);
    PHASAR_LOG_LEVEL(
        DEBUG, "Process normal at target: "
                   << this->ideTabulationProblem.NtoString(Edge.getTarget()));
    d_t d1 = Edge.factAtSource(); // NOLINT
    n_t n = Edge.getTarget();     // NOLINT
    d_t d2 = Edge.factAtTarget(); // NOLINT
    EdgeFunctionPtrType f =       // NOLINT
        IDESolver<AnalysisDomainTy>::jumpFunction(Edge);
    auto SuccessorInsts = IDESolver<AnalysisDomainTy>::ICF->getSuccsOf(n);
    for (auto SuccessorInst : SuccessorInsts) {
      FlowFunctionPtrType flowFunction = // NOLINT
          IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
              .getNormalFlowFunction(n, SuccessorInst);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      std::set<d_t> Res =
          IDESolver<AnalysisDomainTy>::computeNormalFlowFunction(flowFunction,
                                                                 d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      IDESolver<AnalysisDomainTy>::saveEdges(n, SuccessorInst, d2, Res, false);
      for (d_t d3 : Res) {      // NOLINT
        EdgeFunctionPtrType g = // NOLINT
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getNormalEdgeFunction(n, d2, SuccessorInst, d3);
        // add normal PDS rule
        auto d2_k = wali::getKey(d2); // NOLINT
        DKey[d2] = d2_k;
        auto d3_k = wali::getKey(d3); // NOLINT
        DKey[d3] = d3_k;
        auto n_k = wali::getKey(n);             // NOLINT
        auto f_k = wali::getKey(SuccessorInst); // NOLINT
        wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> Wptr;
        Wptr = new JoinLatticeToSemiRingElem<l_t>(
            g, static_cast<JoinLattice<l_t> &>(Problem));
        PHASAR_LOG_LEVEL(DEBUG,
                         "ADD NORMAL RULE: " << Problem.DtoString(d2) << " | "
                                             << Problem.NtoString(n) << " --> "
                                             << Problem.DtoString(d3) << " | "
                                             << Problem.DtoString(SuccessorInst)
                                             << ", " << *Wptr << ")");
        PDS->add_rule(d2_k, n_k, d3_k, f_k, Wptr);
        if (!SRElem.is_valid()) {
          SRElem = Wptr;
        }
        EdgeFunctionPtrType fprime = SuccessorInst->composeWith(g); // NOLINT
        PHASAR_LOG_LEVEL(DEBUG, "Compose: " << g->str() << " * "
                                            << SuccessorInst->str());
        PHASAR_LOG_LEVEL(DEBUG, ' ');
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        IDESolver<AnalysisDomainTy>::propagate(d1, SuccessorInst, d3, fprime,
                                               nullptr, false);
      }
    }
  }

  void processCall(PathEdge<n_t, d_t> Edge) override {
    PHASAR_LOG_LEVEL(DEBUG, "WPDS::processCall");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Call", 1, PAMM_SEVERITY_LEVEL::Full);
    PHASAR_LOG_LEVEL(
        DEBUG, "Process call at target: "
                   << this->ideTabulationProblem.NtoString(Edge.getTarget()));
    d_t d1 = Edge.factAtSource(); // NOLINT
    n_t n = Edge.getTarget();     // NOLINT a call node; line 14...
    d_t d2 = Edge.factAtTarget(); // NOLINT
    EdgeFunctionPtrType f         // NOLINT
        = IDESolver<AnalysisDomainTy>::jumpFunction(Edge);
    std::set<n_t> ReturnSiteNs =
        IDESolver<AnalysisDomainTy>::ICF->getReturnSitesOfCallAt(n);
    std::set<f_t> Callees =
        IDESolver<AnalysisDomainTy>::ICF->getCalleesOfCallAt(n);
    PHASAR_LOG_LEVEL(DEBUG, "Possible callees:");
    for (auto Callee : Callees) {
      PHASAR_LOG_LEVEL(DEBUG, "  " << Callee->getName());
    }
    PHASAR_LOG_LEVEL(DEBUG, "Possible return sites:");
    for (auto Ret : ReturnSiteNs) {
      PHASAR_LOG_LEVEL(DEBUG,
                       "  " << this->ideTabulationProblem.NtoString(Ret));
    }
    // for each possible callee
    for (f_t SCalledProcN : Callees) { // still line 14
      // check if a special summary for the called procedure exists
      FlowFunctionPtrType SpecialSum =
          IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
              .getSummaryFlowFunction(n, SCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (SpecialSum) {
        PHASAR_LOG_LEVEL(DEBUG, "Found and process special summary");
        for (n_t ReturnSiteN : ReturnSiteNs) {
          std::set<d_t> Res =
              IDESolver<AnalysisDomainTy>::computeSummaryFlowFunction(
                  SpecialSum, d1, d2);
          INC_COUNTER("SpecialSummary-FF Application", 1,
                      PAMM_SEVERITY_LEVEL::Full);
          ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<AnalysisDomainTy>::saveEdges(n, ReturnSiteN, d2, Res,
                                                 false);
          for (d_t d3 : Res) { // NOLINT
            EdgeFunctionPtrType SumEdgFnE =
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getSummaryEdgeFunction(n, d2, ReturnSiteN, d3);
            INC_COUNTER("SpecialSummary-EF Queries", 1,
                        PAMM_SEVERITY_LEVEL::Full);
            PHASAR_LOG_LEVEL(DEBUG, "Compose: " << SumEdgFnE->str() << " * "
                                                << f->str());
            PHASAR_LOG_LEVEL(DEBUG, ' ');
            IDESolver<AnalysisDomainTy>::propagate(
                d1, ReturnSiteN, d3, f->composeWith(SumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        FlowFunctionPtrType Function =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getCallFlowFunction(n, SCalledProcN);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<d_t> Res =
            IDESolver<AnalysisDomainTy>::computeCallFlowFunction(Function, d1,
                                                                 d2);
        ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        // for each callee's start point(s)
        std::set<n_t> StartPointsOf =
            IDESolver<AnalysisDomainTy>::ICF->getStartPointsOf(SCalledProcN);
        if (StartPointsOf.empty()) {
          PHASAR_LOG_LEVEL(DEBUG, "Start points of '" +
                                      this->ICF->getFunctionName(SCalledProcN) +
                                      "' currently not available!");
        }
        // if startPointsOf is empty, the called function is a declaration
        for (n_t SP : StartPointsOf) {
          IDESolver<AnalysisDomainTy>::saveEdges(n, SP, d2, Res, true);
          // for each result node of the call-flow function
          for (d_t d3 : Res) { // NOLINT
            // create initial self-loop
            IDESolver<AnalysisDomainTy>::propagate(
                d3, SP, d3, EdgeIdentity<l_t>::getInstance(), n,
                false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            IDESolver<AnalysisDomainTy>::addIncoming(SP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            std::set<typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell>
                EndSumm = std::set<
                    typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell>(
                    IDESolver<AnalysisDomainTy>::endSummary(SP, d3));
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
            for (typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell Entry :
                 EndSumm) {
              n_t eP = Entry.getRowKey();    // NOLINT
              d_t d4 = Entry.getColumnKey(); // NOLINT
              EdgeFunctionPtrType FCalleeSummary = Entry.getValue();
              // for each return site
              for (n_t RetSiteN : ReturnSiteNs) {
                // compute return-flow function
                FlowFunctionPtrType RetFunction =
                    IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                        .getRetFlowFunction(n, SCalledProcN, eP, RetSiteN);
                INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
                std::set<d_t> ReturnedFacts =
                    IDESolver<AnalysisDomainTy>::computeReturnFlowFunction(
                        RetFunction, d3, d4, n, std::set<d_t>{d2});
                ADD_TO_HISTOGRAM("Data-flow facts", returnedFacts.size(), 1,
                                 PAMM_SEVERITY_LEVEL::Full);
                IDESolver<AnalysisDomainTy>::saveEdges(eP, RetSiteN, d4,
                                                       ReturnedFacts, true);
                // for each target value of the function
                for (d_t d5 : ReturnedFacts) { // NOLINT
                  // update the caller-side summary function
                  // get call edge function
                  EdgeFunctionPtrType f4 = // NOLINT
                      IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                          .getCallEdgeFunction(n, d2, SCalledProcN, d3);
                  // add call PDS rule
                  auto d2_k = wali::getKey(d2); // NOLINT
                  DKey[d2] = d2_k;
                  auto d3_k = wali::getKey(d3); // NOLINT
                  DKey[d3] = d3_k;
                  auto n_k = wali::getKey(n);   // NOLINT
                  auto sP_k = wali::getKey(SP); // NOLINT
                  wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> WptrCall(
                      new JoinLatticeToSemiRingElem<l_t>(
                          f4, static_cast<JoinLattice<l_t> &>(Problem)));
                  PHASAR_LOG_LEVEL(DEBUG, "ADD CALL RULE: "
                                              << Problem.DtoString(d2) << ", "
                                              << Problem.NtoString(n) << ", "
                                              << Problem.DtoString(d3) << ", "
                                              << Problem.NtoString(SP) << ", "
                                              << *WptrCall);
                  auto RetSiteNK = wali::getKey(RetSiteN);
                  PDS->add_rule(d2_k, n_k, d3_k, sP_k, RetSiteNK, WptrCall);
                  if (!SRElem.is_valid()) {
                    SRElem = WptrCall;
                  }
                  // get return edge function
                  EdgeFunctionPtrType f5 = // NOLINT
                      IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                          .getReturnEdgeFunction(n, SCalledProcN, eP, d4,
                                                 RetSiteN, d5);
                  // add ret PDS rule
                  auto d4_k = wali::getKey(d4); // NOLINT
                  DKey[d4] = d4_k;
                  auto d5_k = wali::getKey(d5); // NOLINT
                  DKey[d5] = d5_k;
                  wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> WptrRet(
                      new JoinLatticeToSemiRingElem<l_t>(
                          f5, static_cast<JoinLattice<l_t> &>(Problem)));
                  PHASAR_LOG_LEVEL(DEBUG, "ADD RET RULE (CALL): "
                                              << Problem.DtoString(d4) << ", "
                                              << Problem.NtoString(RetSiteN)
                                              << ", " << Problem.DtoString(d5)
                                              << ", " << *WptrRet);
                  std::set<n_t> ExitPointsN =
                      IDESolver<AnalysisDomainTy>::ICF->getExitPointsOf(
                          IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(SP));
                  for (auto ExitPointN : ExitPointsN) {
                    auto ExitPointNK = wali::getKey(ExitPointN);
                    PDS->add_rule(d4_k, ExitPointNK, d5_k, WptrRet);
                  }
                  if (!SRElem.is_valid()) {
                    SRElem = WptrRet;
                  }
                  INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
                  // compose call * calleeSummary * return edge functions
                  PHASAR_LOG_LEVEL(DEBUG, "Compose: " << f5->str() << " * "
                                                      << FCalleeSummary->str()
                                                      << " * " << f4->str());
                  PHASAR_LOG_LEVEL(DEBUG,
                                   "         (return * calleeSummary * call)");
                  EdgeFunctionPtrType fPrime = // NOLINT
                      f4->composeWith(FCalleeSummary)->composeWith(f5);
                  PHASAR_LOG_LEVEL(DEBUG, "       = " << fPrime->str());
                  PHASAR_LOG_LEVEL(DEBUG, ' ');
                  d_t d5_restoredCtx = // NOLINT
                      IDESolver<AnalysisDomainTy>::restoreContextOnReturnedFact(
                          n, d2, d5);
                  // propagte the effects of the entire call
                  PHASAR_LOG_LEVEL(DEBUG, "Compose: " << fPrime->str() << " * "
                                                      << f->str());
                  PHASAR_LOG_LEVEL(DEBUG, ' ');
                  IDESolver<AnalysisDomainTy>::propagate(
                      d1, RetSiteN, d5_restoredCtx, f->composeWith(fPrime), n,
                      false);
                }
              }
            }
          }
        }
      }
      // line 17-19 of Naeem/Lhotak/Rodriguez
      // process intra-procedural flows along call-to-return flow functions
      for (n_t ReturnSiteN : ReturnSiteNs) {
        FlowFunctionPtrType CallToReturnFlowFunction =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getCallToRetFlowFunction(n, ReturnSiteN, Callees);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<d_t> ReturnFacts =
            IDESolver<AnalysisDomainTy>::computeCallToReturnFlowFunction(
                CallToReturnFlowFunction, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", returnFacts.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        IDESolver<AnalysisDomainTy>::saveEdges(n, ReturnSiteN, d2, ReturnFacts,
                                               false);
        for (d_t d3 : ReturnFacts) { // NOLINT
          EdgeFunctionPtrType EdgeFnE =
              IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                  .getCallToRetEdgeFunction(n, d2, ReturnSiteN, d3, Callees);
          // add calltoret PDS rule
          auto d2_k = wali::getKey(d2); // NOLINT
          DKey[d2] = d2_k;
          auto d3_k = wali::getKey(d3); // NOLINT
          DKey[d3] = d3_k;
          auto n_k = wali::getKey(n); // NOLINT
          auto ReturnSiteNK = wali::getKey(ReturnSiteN);
          wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> Wptr(
              new JoinLatticeToSemiRingElem<l_t>(
                  EdgeFnE, static_cast<JoinLattice<l_t> &>(Problem)));
          PHASAR_LOG_LEVEL(DEBUG, "ADD CALLTORET RULE: "
                                      << Problem.DtoString(d2) << " | "
                                      << Problem.NtoString(n) << " --> "
                                      << Problem.DtoString(d3) << ", "
                                      << Problem.NtoString(ReturnSiteN) << ", "
                                      << *Wptr);
          PDS->add_rule(d2_k, n_k, d3_k, ReturnSiteNK, Wptr);
          if (!SRElem.is_valid()) {
            SRElem = Wptr;
          }
          INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          PHASAR_LOG_LEVEL(DEBUG,
                           "Compose: " << EdgeFnE->str() << " * " << f->str());
          PHASAR_LOG_LEVEL(DEBUG, ' ');
          IDESolver<AnalysisDomainTy>::propagate(
              d1, ReturnSiteN, d3, f->composeWith(EdgeFnE), n, false);
        }
      }
    }
  }

  void processExit(PathEdge<n_t, d_t> Edge) override {
    PHASAR_LOG_LEVEL(DEBUG, "WPDS::processExit");
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Exit", 1, PAMM_SEVERITY_LEVEL::Full);
    PHASAR_LOG_LEVEL(
        DEBUG, "Process exit at target: "
                   << this->ideTabulationProblem.NtoString(Edge.getTarget()));
    n_t n = Edge.getTarget(); // NOLINT an exit node; line 21...
    EdgeFunctionPtrType f     // NOLINT
        = IDESolver<AnalysisDomainTy>::jumpFunction(Edge);
    f_t FunctionThatNeedsSummary =
        IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n);
    d_t d1 = Edge.factAtSource(); // NOLINT
    d_t d2 = Edge.factAtTarget(); // NOLINT
    // for each of the method's start points, determine incoming calls
    std::set<n_t> StartPointsOf =
        IDESolver<AnalysisDomainTy>::ICF->getStartPointsOf(
            FunctionThatNeedsSummary);
    std::map<n_t, std::set<d_t>> Inc;
    for (n_t SP : StartPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      IDESolver<AnalysisDomainTy>::addEndSummary(SP, d1, n, d2, f);
      for (auto Entry : IDESolver<AnalysisDomainTy>::incoming(d1, SP)) {
        Inc[Entry.first] = std::set<d_t>{Entry.second};
      }
    }
    IDESolver<AnalysisDomainTy>::printEndSummaryTab();
    IDESolver<AnalysisDomainTy>::printIncomingTab();
    // for each incoming call edge already processed
    //(see processCall(..))
    for (auto Entry : Inc) {
      // line 22
      n_t c = Entry.first; // NOLINT
      // for each return site
      for (n_t RetSiteC :
           IDESolver<AnalysisDomainTy>::ICF->getReturnSitesOfCallAt(c)) {
        // compute return-flow function
        FlowFunctionPtrType RetFunction =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getRetFlowFunction(c, FunctionThatNeedsSummary, n, RetSiteC);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        // for each incoming-call value
        for (d_t d4 : Entry.second) { // NOLINT
          std::set<d_t> Targets =
              IDESolver<AnalysisDomainTy>::computeReturnFlowFunction(
                  RetFunction, d1, d2, c, Entry.second);
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<AnalysisDomainTy>::saveEdges(n, RetSiteC, d2, Targets,
                                                 true);
          std::cout << "RETURN TARGETS: " << Targets.size() << std::endl;
          // for each target value at the return site
          // line 23
          for (d_t d5 : Targets) { // NOLINT
            // compute composed function
            // get call edge function
            EdgeFunctionPtrType f4 = // NOLINT
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getCallEdgeFunction(
                        c, d4,
                        IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n), d1);
            // get return edge function
            EdgeFunctionPtrType f5 = // NOLINT
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getReturnEdgeFunction(
                        c, IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n),
                        n, d2, RetSiteC, d5);
            // add ret PDS rule
            auto d1_k = wali::getKey(d1); // NOLINT
            DKey[d1] = d1_k;
            auto d2_k = wali::getKey(d2); // NOLINT
            DKey[d2] = d2_k;
            auto d5_k = wali::getKey(d5); // NOLINT
            DKey[d5] = d5_k;
            auto n_k = wali::getKey(n); // NOLINT
            wali::ref_ptr<JoinLatticeToSemiRingElem<l_t>> Wptr(
                new JoinLatticeToSemiRingElem<l_t>(
                    f5, static_cast<JoinLattice<l_t> &>(Problem)));
            PHASAR_LOG_LEVEL(DEBUG,
                             "ADD RET RULE: " << Problem.DtoString(d2) << ", "
                                              << Problem.NtoString(n) << ", "
                                              << Problem.DtoString(d5) << ", "
                                              << *Wptr);
            PDS->add_rule(d2_k, n_k, d5_k, Wptr);
            if (!SRElem.is_valid()) {
              SRElem = Wptr;
            }
            INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
            // compose call function * function * return function
            PHASAR_LOG_LEVEL(DEBUG, "Compose: " << f5->str() << " * "
                                                << f->str() << " * "
                                                << f4->str());
            PHASAR_LOG_LEVEL(DEBUG, "         (return * function * call)");
            EdgeFunctionPtrType fPrime // NOLINT
                = f4->composeWith(f)->composeWith(f5);
            PHASAR_LOG_LEVEL(DEBUG, "       = " << fPrime->str());
            PHASAR_LOG_LEVEL(DEBUG, ' ');
            // for each jump function coming into the call, propagate to return
            // site using the composed function
            for (auto ValAndFunc :
                 IDESolver<AnalysisDomainTy>::jumpFn->reverseLookup(c, d4)) {
              EdgeFunctionPtrType f3 = ValAndFunc.second; // NOLINT
              if (!f3->equal_to(IDESolver<AnalysisDomainTy>::allTop)) {
                d_t d3 = ValAndFunc.first; // NOLINT
                d_t d5_restoredCtx =       // NOLINT
                    IDESolver<AnalysisDomainTy>::restoreContextOnReturnedFact(
                        c, d4, d5);
                PHASAR_LOG_LEVEL(DEBUG, "Compose: " << fPrime->str() << " * "
                                                    << f3->str());
                PHASAR_LOG_LEVEL(DEBUG, ' ');
                IDESolver<AnalysisDomainTy>::propagate(
                    d3, RetSiteC, d5_restoredCtx, f3->composeWith(fPrime), c,
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
        Inc.empty() &&
        IDESolver<AnalysisDomainTy>::ideTabulationProblem.isZeroValue(d1)) {
      std::set<n_t> Callers = IDESolver<AnalysisDomainTy>::ICF->getCallersOf(
          FunctionThatNeedsSummary);
      for (n_t C : Callers) {
        for (n_t RetSiteC :
             IDESolver<AnalysisDomainTy>::ICF->getReturnSitesOfCallAt(C)) {
          FlowFunctionPtrType RetFunction =
              IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                  .getRetFlowFunction(C, FunctionThatNeedsSummary, n, RetSiteC);
          INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          std::set<d_t> Targets =
              IDESolver<AnalysisDomainTy>::computeReturnFlowFunction(
                  RetFunction, d1, d2, C,
                  std::set<d_t>{IDESolver<AnalysisDomainTy>::zeroValue});
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          IDESolver<AnalysisDomainTy>::saveEdges(n, RetSiteC, d2, Targets,
                                                 true);
          for (d_t d5 : Targets) {   // NOLINT
            EdgeFunctionPtrType f5 = // NOLINT
                IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                    .getReturnEdgeFunction(
                        C, IDESolver<AnalysisDomainTy>::ICF->getFunctionOf(n),
                        n, d2, RetSiteC, d5);
            INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
            PHASAR_LOG_LEVEL(DEBUG,
                             "Compose: " << f5->str() << " * " << f->str());
            PHASAR_LOG_LEVEL(DEBUG, ' ');
            IDESolver<AnalysisDomainTy>::propagteUnbalancedReturnFlow(
                RetSiteC, d5, f->composeWith(f5), C);
            // register for value processing (2nd IDE phase)
            IDESolver<AnalysisDomainTy>::unbalancedRetSites.insert(RetSiteC);
          }
        }
      }
      // in cases where there are no callers, the return statement would
      // normally not be processed at all; this might be undesirable if
      // the flow function has a side effect such as registering a taint;
      // instead we thus call the return flow function will a null caller
      if (Callers.empty()) {
        FlowFunctionPtrType RetFunction =
            IDESolver<AnalysisDomainTy>::cachedFlowEdgeFunctions
                .getRetFlowFunction(nullptr, FunctionThatNeedsSummary, n,
                                    nullptr);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        RetFunction->computeTargets(d2);
      }
    }
  }

  void doForwardSearch(wali::wfa::WFA &Answer) {
    // Create an automaton to AcceptingState the configuration:
    // <ZeroPDSState, entry>
    for (auto Seed : IDESolver<AnalysisDomainTy>::initialSeeds) {
      wali::Key Entry = wali::getKey(Seed.first);
      Query.addTrans(ZeroPDSState, Entry, AcceptingState, SRElem->one());
    }
    Query.set_initial_state(ZeroPDSState);
    Query.add_final_state(AcceptingState);
    Query.print(std::cout << "BEFORE POSTSTAR\n");
    std::ofstream Before("before_poststar.dot");
    Query.print_dot(Before, true);
    PDS->poststar(Query, Answer);
    Answer.print(std::cout << "AFTER POSTSTAR\n");
    std::ofstream After("after_poststar.dot");
    Answer.print_dot(After, true);
  }

  void doBackwardSearch(wali::Key Node, wali::wfa::WFA &Answer) {
    // Create an automaton to AcceptingState the configurations {n \Gamma^*}
    // Find the set of all return points
    wali::wpds::WpdsStackSymbols Syms;
    PDS->for_each(Syms);
    auto Alloca1 =
        &IDESolver<AnalysisDomainTy>::ICF->getFunction("main")->front().front();
    auto A1Key = wali::getKey(Alloca1);
    // the weight is essential here!
    Query.addTrans(A1Key, Node, AcceptingState, SRElem->one());
    std::set<wali::Key>::iterator It;
    for (It = Syms.returnPoints.begin(); It != Syms.returnPoints.end(); It++) {
      wali::getKeySource(*It)->print(std::cout);
      std::cout << std::endl;
      Query.addTrans(AcceptingState, *It, AcceptingState, SRElem->one());
    }
    Query.set_initial_state(A1Key);
    Query.add_final_state(AcceptingState);
    Query.print(std::cout << "BEFORE PRESTAR\n");
    std::ofstream Before("before_prestar.dot");
    Query.print_dot(Before, true);
    PDS->prestar(Query, Answer);
    Answer.print(std::cout << "AFTER PRESTAR\n");
    std::ofstream After("after_prestar.dot");
    Answer.print_dot(After, true);
  }

  void doBackwardSearch(std::vector<wali::Key> NodeStack,
                        wali::wfa::WFA &Answer) {
    assert(!NodeStack.empty());
    // Create an automaton to AcceptingState the configuration <ZeroPDSState,
    // node_stack>
    wali::Key TempFrom = ZeroPDSState;
    wali::Key TempTo = wali::WALI_EPSILON;
    for (size_t I = 0; I < NodeStack.size(); I++) {
      std::stringstream SStr;
      SStr << "__tmp_state_" << I;
      TempTo = wali::getKey(SStr.str());
      Query.addTrans(TempFrom, NodeStack[I], TempTo, SRElem->one());
      TempFrom = TempTo;
    }
    Query.set_initial_state(ZeroPDSState);
    Query.add_final_state(TempTo);
    PDS->prestar(Query, Answer);
  }

  std::unordered_map<d_t, l_t> resultsAt(n_t Stmt,
                                         bool StripZero = false) override {
    std::unordered_map<d_t, l_t> Results;
    wali::wfa::Trans Goal;
    for (auto Entry : DKey) {
      // Method 1: If query 'stmt' is located within the same function as the
      // starting point
      if (Answer.find(Entry.second, wali::getKey(Stmt), AcceptingState, Goal)) {
        Results.insert(std::make_pair(
            Entry.first,
            static_cast<JoinLatticeToSemiRingElem<l_t> &>(*Goal.weight())
                .F->computeTarget(l_t{})));
      } else {
        // Method 2: If query 'stmt' is located in a different function
        wali::sem_elem_t Ret = SRElem->zero();
        wali::wfa::TransSet Tset;
        wali::wfa::TransSet::iterator TIter;
        Tset = Answer.match(Entry.second, wali::getKey(Stmt));
        for (TIter = Tset.begin(); TIter != Tset.end(); TIter++) {
          wali::wfa::ITrans *Trans = *TIter;
          wali::sem_elem_t Tmp(Answer.getState(Trans->to())->weight());
          Tmp = Tmp->extend(Trans->weight());
          Ret = Ret->combine(Tmp);
        }
        if (!Ret->equal(SRElem->zero())) {
          Results.insert(std::make_pair(
              Entry.first, static_cast<JoinLatticeToSemiRingElem<l_t> &>(*Ret)
                               .F->computeTarget(l_t{})));
        }
      }
    }
    if (StripZero) {
      Results.erase(ZeroValue);
    }
    return Results;
  }

  l_t resultAt(n_t Stmt, d_t Fact) override {
    wali::wfa::Trans Goal;
    if (Answer.find(wali::getKey(Fact), wali::getKey(Stmt), AcceptingState,
                    Goal)) {
      return static_cast<JoinLatticeToSemiRingElem<l_t> &>(*Goal.weight())
          .F->computeTarget(l_t{});
    }
    wali::sem_elem_t Ret = SRElem->zero();
    wali::wfa::TransSet Tset;
    wali::wfa::TransSet::iterator TIter;
    Tset = Answer.match(wali::getKey(Fact), wali::getKey(Stmt));
    for (TIter = Tset.begin(); TIter != Tset.end(); TIter++) {
      wali::wfa::ITrans *Trans = *TIter;
      wali::sem_elem_t Tmp(Answer.getState(Trans->to())->weight());
      Tmp = Tmp->extend(Trans->weight());
      Ret = Ret->combine(Tmp);
    }
    if (!Ret->equal(SRElem->zero())) {
      return static_cast<JoinLatticeToSemiRingElem<l_t> &>(*Ret)
          .F->computeTarget(l_t{});
    }
    throw std::runtime_error("Requested invalid fact!");
  }
};

} // namespace psr

#endif
