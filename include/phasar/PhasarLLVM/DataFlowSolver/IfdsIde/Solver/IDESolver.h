/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IDESolver.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IDESOLVER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IDESOLVER_H_

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "nlohmann/json.hpp"

#include "boost/algorithm/string/trim.hpp"

#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowEdgeFunctionCache.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSToIDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/JoinHandlingNode.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/JumpFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/LinkedNode.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/PathEdge.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/DOTGraph.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Table.h"

namespace psr {

// Forward declare the Transformation
template <typename AnalysisDomainTy, typename Container>
class IFDSToIDETabulationProblem;

struct NoIFDSExtension {};

template <typename AnalysisDomainTy, typename Container> struct IFDSExtension {
  using BaseAnalysisDomain = typename AnalysisDomainTy::BaseAnalysisDomain;

  IFDSExtension(IFDSTabulationProblem<BaseAnalysisDomain, Container> &Problem)
      : TransformedProblem(
            std::make_unique<
                IFDSToIDETabulationProblem<BaseAnalysisDomain, Container>>(
                Problem)) {}

  std::unique_ptr<IFDSToIDETabulationProblem<BaseAnalysisDomain, Container>>
      TransformedProblem;
};

/**
 * Solves the given IDETabulationProblem as described in the 1996 paper by
 * Sagiv, Horwitz and Reps. To solve the problem, call solve(). Results
 * can then be queried by using resultAt() and resultsAt().
 */
template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>,
          bool = is_analysis_domain_extensions<AnalysisDomainTy>::value>
class IDESolver
    : protected std::conditional_t<
          is_analysis_domain_extensions<AnalysisDomainTy>::value,
          IFDSExtension<AnalysisDomainTy, Container>, NoIFDSExtension> {
public:
  using ProblemTy = IDETabulationProblem<AnalysisDomainTy, Container>;
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

  IDESolver(IDETabulationProblem<AnalysisDomainTy, Container> &Problem)
      : IDEProblem(Problem), ZeroValue(Problem.getZeroValue()),
        ICF(Problem.getICFG()), SolverConfig(Problem.getIFDSIDESolverConfig()),
        cachedFlowEdgeFunctions(Problem), allTop(Problem.allTopFunction()),
        jumpFn(std::make_shared<JumpFunctions<AnalysisDomainTy, Container>>(
            allTop, IDEProblem)),
        initialSeeds(Problem.initialSeeds()) {}

  IDESolver(const IDESolver &) = delete;
  IDESolver &operator=(const IDESolver &) = delete;
  IDESolver(IDESolver &&) = delete;
  IDESolver &operator=(IDESolver &&) = delete;

  virtual ~IDESolver() = default;

  nlohmann::json getAsJson() {
    using TableCell = typename Table<n_t, d_t, l_t>::Cell;
    const static std::string DataFlowID = "DataFlow";
    nlohmann::json J;
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      J[DataFlowID] = "EMPTY";
    } else {
      std::vector<TableCell> cells(results.begin(), results.end());
      sort(cells.begin(), cells.end(), [](TableCell a, TableCell b) {
        return a.getRowKey() < b.getRowKey();
      });
      n_t curr;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].getRowKey();
        std::string n = IDEProblem.NtoString(cells[i].getRowKey());
        boost::algorithm::trim(n);
        std::string node =
            ICF->getFunctionName(ICF->getFunctionOf(curr)) + "::" + n;
        J[DataFlowID][node];
        std::string fact = IDEProblem.DtoString(cells[i].getColumnKey());
        boost::algorithm::trim(fact);
        std::string value = IDEProblem.LtoString(cells[i].getValue());
        boost::algorithm::trim(value);
        J[DataFlowID][node]["Facts"] += {fact, value};
      }
    }
    return J;
  }

  /**
   * @brief Runs the solver on the configured problem. This can take some time.
   */
  virtual void solve() {
    PAMM_GET_INSTANCE;
    REG_COUNTER("Gen facts", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Kill facts", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Summary-reuse", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Intra Path Edges", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("Inter Path Edges", 0, PAMM_SEVERITY_LEVEL::Core);
    REG_COUNTER("FF Queries", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("EF Queries", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Value Propagation", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Value Computation", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("SpecialSummary-FF Application", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("SpecialSummary-EF Queries", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("JumpFn Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Process Call", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Process Normal", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Process Exit", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("[Calls] getPointsToSet", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_HISTOGRAM("Data-flow facts", PAMM_SEVERITY_LEVEL::Full);
    REG_HISTOGRAM("Points-to", PAMM_SEVERITY_LEVEL::Full);

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                      << "IDE solver is solving the specified problem";
                  BOOST_LOG_SEV(lg::get(), INFO)
                  << "Submit initial seeds, construct exploded super graph");
    // computations starting here
    START_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    // We start our analysis and construct exploded supergraph
    submitInitialSeeds();
    STOP_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    if (SolverConfig.computeValues()) {
      START_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
      // Computing the final values for the edge functions
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Compute the final values according to the edge functions");
      computeValues();
      STOP_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << "Problem solved");
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      computeAndPrintStatistics();
    }
    if (SolverConfig.emitESG()) {
      emitESGAsDot();
    }
  }

  /**
   * Returns the V-type result for the given value at the given statement.
   * TOP values are never returned.
   */
  [[nodiscard]] virtual l_t resultAt(n_t stmt, d_t value) {
    return valtab.get(stmt, value);
  }

  /**
   * Returns the resulting environment for the given statement.
   * The artificial zero value can be automatically stripped.
   * TOP values are never returned.
   */
  [[nodiscard]] virtual std::unordered_map<d_t, l_t>
  resultsAt(n_t stmt, bool stripZero = false) /*TODO const*/ {
    std::unordered_map<d_t, l_t> result = valtab.row(stmt);
    if (stripZero) {
      for (auto it = result.begin(); it != result.end();) {
        if (IDEProblem.isZeroValue(it->first)) {
          it = result.erase(it);
        } else {
          ++it;
        }
      }
    }
    return result;
  }

  virtual void emitTextReport(std::ostream &OS = std::cout) {
    IDEProblem.emitTextReport(getSolverResults(), OS);
  }

  virtual void emitGraphicalReport(std::ostream &OS = std::cout) {
    IDEProblem.emitGraphicalReport(getSolverResults(), OS);
  }

  virtual void dumpResults(std::ostream &OS = std::cout) {
    PAMM_GET_INSTANCE;
    START_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
    OS << "\n***************************************************************\n"
       << "*                  Raw IDESolver results                      *\n"
       << "***************************************************************\n";
    auto cells = this->valtab.cellVec();
    if (cells.empty()) {
      OS << "No results computed!" << std::endl;
    } else {
      llvmValueIDLess llvmIDLess;
      std::sort(
          cells.begin(), cells.end(),
          [&llvmIDLess](const auto &a, const auto &b) {
            if constexpr (std::is_same_v<n_t, const llvm::Instruction *>) {
              return llvmIDLess(a.getRowKey(), b.getRowKey());
            } else {
              // If non-LLVM IR is used
              return a.getRowKey() < b.getRowKey();
            }
          });
      n_t prev = n_t{};
      n_t curr = n_t{};
      f_t prevFn = f_t{};
      f_t currFn = f_t{};
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].getRowKey();
        currFn = ICF->getFunctionOf(curr);
        if (prevFn != currFn) {
          prevFn = currFn;
          OS << "\n\n============ Results for function '" +
                    ICF->getFunctionName(currFn) + "' ============\n";
        }
        if (prev != curr) {
          prev = curr;
          std::string NString = IDEProblem.NtoString(curr);
          std::string line(NString.size(), '-');
          OS << "\n\nN: " << NString << "\n---" << line << '\n';
        }
        OS << "\tD: " << IDEProblem.DtoString(cells[i].getColumnKey())
           << " | V: " << IDEProblem.LtoString(cells[i].getValue()) << '\n';
      }
    }
    OS << '\n';
    STOP_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
  }

  void dumpAllInterPathEdges() {
    std::cout << "COMPUTED INTER PATH EDGES" << std::endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (const auto &cell : interpe) {
      std::cout << "FROM" << std::endl;
      IDEProblem.printNode(std::cout, cell.getRowKey());
      std::cout << "TO" << std::endl;
      IDEProblem.printNode(std::cout, cell.getColumnKey());
      std::cout << "FACTS" << std::endl;
      for (const auto &fact : cell.getValue()) {
        std::cout << "fact" << std::endl;
        IDEProblem.printFlowFact(std::cout, fact.first);
        std::cout << "produces" << std::endl;
        for (const auto &out : fact.second) {
          IDEProblem.printFlowFact(std::cout, out);
        }
      }
    }
  }

  void dumpAllIntraPathEdges() {
    std::cout << "COMPUTED INTRA PATH EDGES" << std::endl;
    auto intrape = this->computedIntraPathEdges.cellSet();
    for (auto &cell : intrape) {
      std::cout << "FROM" << std::endl;
      IDEProblem.printNode(std::cout, cell.getRowKey());
      std::cout << "TO" << std::endl;
      IDEProblem.printNode(std::cout, cell.getColumnKey());
      std::cout << "FACTS" << std::endl;
      for (auto &fact : cell.getValue()) {
        std::cout << "fact" << std::endl;
        IDEProblem.printFlowFact(std::cout, fact.first);
        std::cout << "produces" << std::endl;
        for (auto &out : fact.second) {
          IDEProblem.printFlowFact(std::cout, out);
        }
      }
    }
  }

  SolverResults<n_t, d_t, l_t> getSolverResults() {
    return SolverResults<n_t, d_t, l_t>(this->valtab,
                                        IDEProblem.getZeroValue());
  }

protected:
  // have a shared point to allow for a copy constructor of IDESolver
  IDETabulationProblem<AnalysisDomainTy, Container> &IDEProblem;
  d_t ZeroValue;
  const i_t *ICF;
  IFDSIDESolverConfig &SolverConfig;
  unsigned PathEdgeCount = 0;

  FlowEdgeFunctionCache<AnalysisDomainTy, Container> cachedFlowEdgeFunctions;

  Table<n_t, n_t, std::map<d_t, Container>> computedIntraPathEdges;

  Table<n_t, n_t, std::map<d_t, Container>> computedInterPathEdges;

  EdgeFunctionPtrType allTop;

  std::shared_ptr<JumpFunctions<AnalysisDomainTy, Container>> jumpFn;

  std::map<std::tuple<n_t, d_t, n_t, d_t>, std::vector<EdgeFunctionPtrType>>
      intermediateEdgeFunctions;

  // stores summaries that were queried before they were computed
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<n_t, d_t, Table<n_t, d_t, EdgeFunctionPtrType>> endsummarytab;

  // edges going along calls
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<n_t, d_t, std::map<n_t, Container>> incomingtab;

  // stores the return sites (inside callers) to which we have unbalanced
  // returns if SolverConfig.followReturnPastSeeds is enabled
  std::set<n_t> unbalancedRetSites;

  std::map<n_t, std::set<d_t>> initialSeeds;

  Table<n_t, d_t, l_t> valtab;

  std::map<std::pair<n_t, d_t>, size_t> fSummaryReuse;

  // When transforming an IFDSTabulationProblem into an IDETabulationProblem,
  // we need to allocate dynamically, otherwise the objects lifetime runs out
  // - as a modifiable r-value reference created here that should be stored in
  // a modifiable l-value reference within the IDESolver implementation leads
  // to (massive) undefined behavior (and nightmares):
  // https://stackoverflow.com/questions/34240794/understanding-the-warning-binding-r-value-to-l-value-reference
  template <typename IFDSAnalysisDomainTy,
            typename = std::enable_if_t<
                is_analysis_domain_extensions<AnalysisDomainTy>::value,
                IFDSAnalysisDomainTy>>
  IDESolver(IFDSTabulationProblem<IFDSAnalysisDomainTy, Container> &Problem)
      : IFDSExtension<AnalysisDomainTy, Container>(Problem),
        IDEProblem(*this->TransformedProblem),
        ZeroValue(IDEProblem.getZeroValue()), ICF(IDEProblem.getICFG()),
        SolverConfig(IDEProblem.getIFDSIDESolverConfig()),
        cachedFlowEdgeFunctions(IDEProblem),
        allTop(IDEProblem.allTopFunction()),
        jumpFn(std::make_shared<JumpFunctions<AnalysisDomainTy, Container>>(
            allTop, IDEProblem)),
        initialSeeds(IDEProblem.initialSeeds()) {}

  /**
   * Lines 13-20 of the algorithm; processing a call site in the caller's
   * context.
   *
   * For each possible callee, registers incoming call edges.
   * Also propagates call-to-return flows and summarized callee flows within
   *the caller.
   *
   * 	The following cases must be considered and handled:
   *		1. Process as usual and just process the call
   *		2. Create a new summary for that function (which shall be done
   *       by the problem)
   *		3. Just use an existing summary provided by the problem
   *		4. If a special function is called, use a special summary
   *       function
   *
   * @param edge an edge whose target node resembles a method call
   */
  virtual void processCall(const PathEdge<n_t, d_t> edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Call", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process call at target: "
                  << IDEProblem.NtoString(edge.getTarget()));
    d_t d1 = edge.factAtSource();
    n_t n = edge.getTarget(); // a call node; line 14...
    d_t d2 = edge.factAtTarget();
    EdgeFunctionPtrType f = jumpFunction(edge);
    const std::set<n_t> returnSiteNs = ICF->getReturnSitesOfCallAt(n);
    const std::set<f_t> callees = ICF->getCalleesOfCallAt(n);

    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg::get(), DEBUG) << "Possible callees:";
        for (auto callee
             : callees) {
          BOOST_LOG_SEV(lg::get(), DEBUG) << "  " << callee->getName().str();
        } BOOST_LOG_SEV(lg::get(), DEBUG)
        << "Possible return sites:";
        for (auto ret
             : returnSiteNs) {
          BOOST_LOG_SEV(lg::get(), DEBUG) << "  " << IDEProblem.NtoString(ret);
        } BOOST_LOG_SEV(lg::get(), DEBUG)
        << ' ');

    // for each possible callee
    for (f_t sCalledProcN : callees) { // still line 14
      // check if a special summary for the called procedure exists
      FlowFunctionPtrType specialSum =
          cachedFlowEdgeFunctions.getSummaryFlowFunction(n, sCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (specialSum) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Found and process special summary");
        for (n_t returnSiteN : returnSiteNs) {
          container_type res = computeSummaryFlowFunction(specialSum, d1, d2);
          INC_COUNTER("SpecialSummary-FF Application", 1,
                      PAMM_SEVERITY_LEVEL::Full);
          ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, returnSiteN, d2, res, false);
          for (d_t d3 : res) {
            EdgeFunctionPtrType sumEdgFnE =
                cachedFlowEdgeFunctions.getSummaryEdgeFunction(n, d2,
                                                               returnSiteN, d3);
            INC_COUNTER("SpecialSummary-EF Queries", 1,
                        PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(
                BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Queried Summary Edge Function: " << sumEdgFnE->str();
                BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Compose: " << sumEdgFnE->str() << " * " << f->str();
                BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
            propagate(d1, returnSiteN, d3, f->composeWith(sumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        FlowFunctionPtrType function =
            cachedFlowEdgeFunctions.getCallFlowFunction(n, sCalledProcN);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        container_type res = computeCallFlowFunction(function, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        // for each callee's start point(s)
        std::set<n_t> startPointsOf = ICF->getStartPointsOf(sCalledProcN);
        if (startPointsOf.empty()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                            << "Start points of '" +
                                   ICF->getFunctionName(sCalledProcN) +
                                   "' currently not available!";
                        BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
        }
        // if startPointsOf is empty, the called function is a declaration
        for (n_t sP : startPointsOf) {
          saveEdges(n, sP, d2, res, true);
          // for each result node of the call-flow function
          for (d_t d3 : res) {
            using TableCell =
                typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell;
            // create initial self-loop
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Create initial self-loop with D: "
                          << IDEProblem.DtoString(d3));
            propagate(d3, sP, d3, EdgeIdentity<l_t>::getInstance(), n,
                      false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            addIncoming(sP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            // const std::set<TableCell> endSumm(endSummary(sP, d3));
            // std::cout << "ENDSUMM" << std::endl;
            // std::cout << "Size: " << endSumm.size() << std::endl;
            // std::cout << "sP: " << IDEProblem.NtoString(sP)
            //           << "\nd3: " << IDEProblem.DtoString(d3)
            //           << std::endl;
            // printEndSummaryTab();
            // still line 15.2 of Naeem/Lhotak/Rodriguez
            // for each already-queried exit value <eP,d4> reachable from
            // <sP,d3>, create new caller-side jump functions to the return
            // sites because we have observed a potentially new incoming
            // edge into <sP,d3>
            for (const TableCell entry : endSummary(sP, d3)) {
              n_t eP = entry.getRowKey();
              d_t d4 = entry.getColumnKey();
              EdgeFunctionPtrType fCalleeSummary = entry.getValue();
              // for each return site
              for (n_t retSiteN : returnSiteNs) {
                // compute return-flow function
                FlowFunctionPtrType retFunction =
                    cachedFlowEdgeFunctions.getRetFlowFunction(n, sCalledProcN,
                                                               eP, retSiteN);
                INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
                const container_type returnedFacts = computeReturnFlowFunction(
                    retFunction, d3, d4, n, Container{d2});
                ADD_TO_HISTOGRAM("Data-flow facts", returnedFacts.size(), 1,
                                 PAMM_SEVERITY_LEVEL::Full);
                saveEdges(eP, retSiteN, d4, returnedFacts, true);
                // for each target value of the function
                for (d_t d5 : returnedFacts) {
                  // update the caller-side summary function
                  // get call edge function
                  EdgeFunctionPtrType f4 =
                      cachedFlowEdgeFunctions.getCallEdgeFunction(
                          n, d2, sCalledProcN, d3);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "Queried Call Edge Function: " << f4->str());
                  // get return edge function
                  EdgeFunctionPtrType f5 =
                      cachedFlowEdgeFunctions.getReturnEdgeFunction(
                          n, sCalledProcN, eP, d4, retSiteN, d5);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "Queried Return Edge Function: "
                                << f5->str());
                  if (SolverConfig.emitESG()) {
                    for (auto sP : ICF->getStartPointsOf(sCalledProcN)) {
                      intermediateEdgeFunctions[std::make_tuple(n, d2, sP, d3)]
                          .push_back(f4);
                    }
                    intermediateEdgeFunctions[std::make_tuple(eP, d4, retSiteN,
                                                              d5)]
                        .push_back(f5);
                  }
                  INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
                  // compose call * calleeSummary * return edge functions
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                    << "Compose: " << f5->str() << " * "
                                    << fCalleeSummary->str() << " * "
                                    << f4->str();
                                BOOST_LOG_SEV(lg::get(), DEBUG)
                                << "         (return * calleeSummary * call)");
                  EdgeFunctionPtrType fPrime =
                      f4->composeWith(fCalleeSummary)->composeWith(f5);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                    << "       = " << fPrime->str();
                                BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
                  d_t d5_restoredCtx = restoreContextOnReturnedFact(n, d2, d5);
                  // propagte the effects of the entire call
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                    << "Compose: " << fPrime->str() << " * "
                                    << f->str();
                                BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
                  propagate(d1, retSiteN, d5_restoredCtx,
                            f->composeWith(fPrime), n, false);
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
            cachedFlowEdgeFunctions.getCallToRetFlowFunction(n, returnSiteN,
                                                             callees);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        container_type returnFacts =
            computeCallToReturnFlowFunction(callToReturnFlowFunction, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", returnFacts.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        saveEdges(n, returnSiteN, d2, returnFacts, false);
        for (d_t d3 : returnFacts) {
          EdgeFunctionPtrType edgeFnE =
              cachedFlowEdgeFunctions.getCallToRetEdgeFunction(
                  n, d2, returnSiteN, d3, callees);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "Queried Call-to-Return Edge Function: "
                        << edgeFnE->str());
          if (SolverConfig.emitESG()) {
            intermediateEdgeFunctions[std::make_tuple(n, d2, returnSiteN, d3)]
                .push_back(edgeFnE);
          }
          INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          auto fPrime = f->composeWith(edgeFnE);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                            << "Compose: " << edgeFnE->str() << " * "
                            << f->str() << " = " << fPrime->str();
                        BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
          propagate(d1, returnSiteN, d3, fPrime, n, false);
        }
      }
    }
  }

  /**
   * Lines 33-37 of the algorithm.
   * Simply propagate normal, intra-procedural flows.
   * @param edge
   */
  virtual void processNormalFlow(const PathEdge<n_t, d_t> edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Normal", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process normal at target: "
                  << IDEProblem.NtoString(edge.getTarget()));
    d_t d1 = edge.factAtSource();
    n_t n = edge.getTarget();
    d_t d2 = edge.factAtTarget();
    EdgeFunctionPtrType f = jumpFunction(edge);
    for (const auto fn : ICF->getSuccsOf(n)) {
      FlowFunctionPtrType flowFunction =
          cachedFlowEdgeFunctions.getNormalFlowFunction(n, fn);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      const container_type res =
          computeNormalFlowFunction(flowFunction, d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      saveEdges(n, fn, d2, res, false);
      for (d_t d3 : res) {
        EdgeFunctionPtrType g =
            cachedFlowEdgeFunctions.getNormalEdgeFunction(n, d2, fn, d3);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Queried Normal Edge Function: " << g->str());
        EdgeFunctionPtrType fprime = f->composeWith(g);
        if (SolverConfig.emitESG()) {
          intermediateEdgeFunctions[std::make_tuple(n, d2, fn, d3)].push_back(
              g);
        }
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Compose: " << g->str() << " * " << f->str()
                          << " = " << fprime->str();
                      BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        propagate(d1, fn, d3, fprime, nullptr, false);
      }
    }
  }

  void propagateValueAtStart(const std::pair<n_t, d_t> nAndD, n_t n) {
    PAMM_GET_INSTANCE;
    d_t d = nAndD.second;
    f_t p = ICF->getFunctionOf(n);
    for (const n_t c : ICF->getCallsFromWithin(p)) {
      auto lookupResults = jumpFn->forwardLookup(d, c);
      if (!lookupResults) {
        continue;
      }
      for (auto entry : lookupResults->get()) {
        d_t dPrime = entry.first;
        EdgeFunctionPtrType fPrime = entry.second;
        n_t sP = n;
        l_t value = val(sP, d);
        INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
        propagateValue(c, dPrime, fPrime->computeTarget(value));
      }
    }
  }

  void propagateValueAtCall(const std::pair<n_t, d_t> nAndD, n_t n) {
    PAMM_GET_INSTANCE;
    d_t d = nAndD.second;
    for (const f_t q : ICF->getCalleesOfCallAt(n)) {
      FlowFunctionPtrType callFlowFunction =
          cachedFlowEdgeFunctions.getCallFlowFunction(n, q);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      for (const d_t dPrime : callFlowFunction->computeTargets(d)) {
        EdgeFunctionPtrType edgeFn =
            cachedFlowEdgeFunctions.getCallEdgeFunction(n, d, q, dPrime);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Queried Call Edge Function: " << edgeFn->str());
        if (SolverConfig.emitESG()) {
          for (const auto sP : ICF->getStartPointsOf(q)) {
            intermediateEdgeFunctions[std::make_tuple(n, d, sP, dPrime)]
                .push_back(edgeFn);
          }
        }
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        for (const n_t startPoint : ICF->getStartPointsOf(q)) {
          INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
          propagateValue(startPoint, dPrime, edgeFn->computeTarget(val(n, d)));
        }
      }
    }
  }

  void propagateValue(n_t nHashN, d_t nHashD, const l_t &l) {
    l_t valNHash = val(nHashN, nHashD);
    l_t lPrime = joinValueAt(nHashN, nHashD, valNHash, l);
    if (!(lPrime == valNHash)) {
      setVal(nHashN, nHashD, std::move(lPrime));
      valuePropagationTask(std::pair<n_t, d_t>(nHashN, nHashD));
    }
  }

  l_t val(n_t nHashN, d_t nHashD) {
    if (valtab.contains(nHashN, nHashD)) {
      return valtab.get(nHashN, nHashD);
    } else {
      // implicitly initialized to top; see line [1] of Fig. 7 in SRH96 paper
      return IDEProblem.topElement();
    }
  }

  void setVal(n_t nHashN, d_t nHashD, l_t l) {
    LOG_IF_ENABLE([&]() {
      BOOST_LOG_SEV(lg::get(), DEBUG)
          << "Function : " << ICF->getFunctionOf(nHashN)->getName().str();
      BOOST_LOG_SEV(lg::get(), DEBUG)
          << "Inst.    : " << IDEProblem.NtoString(nHashN);
      BOOST_LOG_SEV(lg::get(), DEBUG)
          << "Fact     : " << IDEProblem.DtoString(nHashD);
      BOOST_LOG_SEV(lg::get(), DEBUG)
          << "Value    : " << IDEProblem.LtoString(l);
      BOOST_LOG_SEV(lg::get(), DEBUG) << ' ';
    }());
    // TOP is the implicit default value which we do not need to store.
    // if (l == IDEProblem.topElement()) {
    // do not store top values
    // valtab.remove(nHashN, nHashD);
    // } else {
    valtab.insert(nHashN, nHashD, std::move(l));
    // }
  }

  EdgeFunctionPtrType jumpFunction(const PathEdge<n_t, d_t> edge) {
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg::get(), DEBUG) << " ";
        BOOST_LOG_SEV(lg::get(), DEBUG) << "JumpFunctions Forward-Lookup:";
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "   Source D: " << IDEProblem.DtoString(edge.factAtSource());
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "   Target N: " << IDEProblem.NtoString(edge.getTarget());
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "   Target D: " << IDEProblem.DtoString(edge.factAtTarget()));

    auto fwdLookupRes =
        jumpFn->forwardLookup(edge.factAtSource(), edge.getTarget());
    if (fwdLookupRes) {
      auto &ref = fwdLookupRes->get();
      if (auto Find = std::find_if(ref.begin(), ref.end(),
                                   [edge](const auto &Pair) {
                                     return edge.factAtTarget() == Pair.first;
                                   });
          Find != ref.end()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "  => EdgeFn: " << Find->second->str();
                      BOOST_LOG_SEV(lg::get(), DEBUG) << " ");
        return Find->second;
      }
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "  => EdgeFn: " << allTop->str();
                  BOOST_LOG_SEV(lg::get(), DEBUG) << " ");
    // JumpFn initialized to all-top, see line [2] in SRH96 paper
    return allTop;
  }

  void addEndSummary(n_t sP, d_t d1, n_t eP, d_t d2, EdgeFunctionPtrType f) {
    // note: at this point we don't need to join with a potential previous f
    // because f is a jump function, which is already properly joined
    // within propagate(..)
    endsummarytab.get(sP, d1).insert(eP, d2, std::move(f));
  }

  // should be made a callable at some point
  void pathEdgeProcessingTask(const PathEdge<n_t, d_t> edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("JumpFn Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "-------------------------------------------- " << PathEdgeCount
            << ". Path Edge --------------------------------------------";
        BOOST_LOG_SEV(lg::get(), DEBUG) << ' ';
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "Process " << PathEdgeCount << ". path edge:";
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "< D source: " << IDEProblem.DtoString(edge.factAtSource()) << " ;";
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "  N target: " << IDEProblem.NtoString(edge.getTarget()) << " ;";
        BOOST_LOG_SEV(lg::get(), DEBUG)
        << "  D target: " << IDEProblem.DtoString(edge.factAtTarget()) << " >";
        BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');

    if (!ICF->isCallStmt(edge.getTarget())) {
      if (ICF->isExitStmt(edge.getTarget())) {
        processExit(edge);
      }
      if (!ICF->getSuccsOf(edge.getTarget()).empty()) {
        processNormalFlow(edge);
      }
    } else {
      processCall(edge);
    }
  }

  // should be made a callable at some point
  void valuePropagationTask(const std::pair<n_t, d_t> nAndD) {
    n_t n = nAndD.first;
    // our initial seeds are not necessarily method-start points but here they
    // should be treated as such the same also for unbalanced return sites in
    // an unbalanced problem
    if (ICF->isStartPoint(n) || initialSeeds.count(n) ||
        unbalancedRetSites.count(n)) {
      propagateValueAtStart(nAndD, n);
    }
    if (ICF->isCallStmt(n)) {
      propagateValueAtCall(nAndD, n);
    }
  }

  // should be made a callable at some point
  void valueComputationTask(const std::vector<n_t> &values) {
    PAMM_GET_INSTANCE;
    for (n_t n : values) {
      for (n_t sP : ICF->getStartPointsOf(ICF->getFunctionOf(n))) {
        using TableCell = typename Table<d_t, d_t, EdgeFunctionPtrType>::Cell;
        Table<d_t, d_t, EdgeFunctionPtrType> lookupByTarget;
        lookupByTarget = jumpFn->lookupByTarget(n);
        for (const TableCell &sourceValTargetValAndFunction :
             lookupByTarget.cellSet()) {
          d_t dPrime = sourceValTargetValAndFunction.getRowKey();
          d_t d = sourceValTargetValAndFunction.getColumnKey();
          EdgeFunctionPtrType fPrime = sourceValTargetValAndFunction.getValue();
          l_t targetVal = val(sP, dPrime);
          setVal(n, d,
                 IDEProblem.join(val(n, d),
                                 fPrime->computeTarget(std::move(targetVal))));
          INC_COUNTER("Value Computation", 1, PAMM_SEVERITY_LEVEL::Full);
        }
      }
    }
  }

  virtual void saveEdges(n_t sourceNode, n_t sinkStmt, d_t sourceVal,
                         const container_type &destVals, bool interP) {
    if (!SolverConfig.recordEdges()) {
      return;
    }
    Table<n_t, n_t, std::map<d_t, container_type>> &tgtMap =
        (interP) ? computedInterPathEdges : computedIntraPathEdges;
    tgtMap.get(sourceNode, sinkStmt)[sourceVal].insert(destVals.begin(),
                                                       destVals.end());
  }

  /**
   * Computes the final values for edge functions.
   */
  void computeValues() {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "Start computing values");
    // Phase II(i)
    std::map<n_t, std::set<d_t>> allSeeds(initialSeeds);
    for (n_t unbalancedRetSite : unbalancedRetSites) {
      if (allSeeds[unbalancedRetSite].empty()) {
        allSeeds.emplace(unbalancedRetSite, std::set<d_t>({ZeroValue}));
      }
    }
    // do processing
    for (const auto &seed : allSeeds) {
      n_t startPoint = seed.first;
      for (d_t val : seed.second) {
        // initialize the initial seeds with the top element as we have no
        // information at the beginning of the value computation problem
        setVal(startPoint, val, IDEProblem.topElement());
        std::pair<n_t, d_t> superGraphNode(startPoint, val);
        valuePropagationTask(superGraphNode);
      }
    }
    // Phase II(ii)
    // we create an array of all nodes and then dispatch fractions of this
    // array to multiple threads
    const std::set<n_t> allNonCallStartNodes = ICF->allNonCallStartNodes();
    valueComputationTask(
        {allNonCallStartNodes.begin(), allNonCallStartNodes.end()});
  }

  /**
   * Schedules the processing of initial seeds, initiating the analysis.
   * Clients should only call this methods if performing synchronization on
   * their own. Normally, solve() should be called instead.
   */
  void submitInitialSeeds() {
    PAMM_GET_INSTANCE;
    for (const auto &[StartPoint, Facts] : initialSeeds) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Start point: " << IDEProblem.NtoString(StartPoint));
      for (const auto &Fact : Facts) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "\tFact: " << IDEProblem.DtoString(Fact);
                      BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
        if (!IDEProblem.isZeroValue(Fact)) {
          INC_COUNTER("Gen facts", 1, PAMM_SEVERITY_LEVEL::Core);
        }
        propagate(ZeroValue, StartPoint, Fact, EdgeIdentity<l_t>::getInstance(),
                  nullptr, false);
      }
      jumpFn->addFunction(ZeroValue, StartPoint, ZeroValue,
                          EdgeIdentity<l_t>::getInstance());
    }
  }

  /**
   * Lines 21-32 of the algorithm.
   *
   * Stores callee-side summaries.
   * Also, at the side of the caller, propagates intra-procedural flows to
   * return sites using those newly computed summaries.
   *
   * @param edge an edge whose target node resembles a method exit
   */
  virtual void processExit(const PathEdge<n_t, d_t> edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Exit", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process exit at target: "
                  << IDEProblem.NtoString(edge.getTarget()));
    n_t n = edge.getTarget(); // an exit node; line 21...
    EdgeFunctionPtrType f = jumpFunction(edge);
    f_t functionThatNeedsSummary = ICF->getFunctionOf(n);
    d_t d1 = edge.factAtSource();
    d_t d2 = edge.factAtTarget();
    // for each of the method's start points, determine incoming calls
    const std::set<n_t> startPointsOf =
        ICF->getStartPointsOf(functionThatNeedsSummary);
    std::map<n_t, container_type> inc;
    for (n_t sP : startPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      addEndSummary(sP, d1, n, d2, f);
      for (auto entry : incoming(d1, sP)) {
        inc[entry.first] = Container{entry.second};
      }
    }
    printEndSummaryTab();
    printIncomingTab();
    // for each incoming call edge already processed
    //(see processCall(..))
    for (auto entry : inc) {
      // line 22
      n_t c = entry.first;
      // for each return site
      for (n_t retSiteC : ICF->getReturnSitesOfCallAt(c)) {
        // compute return-flow function
        FlowFunctionPtrType retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                c, functionThatNeedsSummary, n, retSiteC);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        // for each incoming-call value
        for (d_t d4 : entry.second) {
          const container_type targets =
              computeReturnFlowFunction(retFunction, d1, d2, c, entry.second);
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, retSiteC, d2, targets, true);
          // for each target value at the return site
          // line 23
          for (d_t d5 : targets) {
            // compute composed function
            // get call edge function
            EdgeFunctionPtrType f4 =
                cachedFlowEdgeFunctions.getCallEdgeFunction(
                    c, d4, ICF->getFunctionOf(n), d1);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Queried Call Edge Function: " << f4->str());
            // get return edge function
            EdgeFunctionPtrType f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, ICF->getFunctionOf(n), n, d2, retSiteC, d5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Queried Return Edge Function: " << f5->str());
            if (SolverConfig.emitESG()) {
              for (auto sP : ICF->getStartPointsOf(ICF->getFunctionOf(n))) {
                intermediateEdgeFunctions[std::make_tuple(c, d4, sP, d1)]
                    .push_back(f4);
              }
              intermediateEdgeFunctions[std::make_tuple(n, d2, retSiteC, d5)]
                  .push_back(f5);
            }
            INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
            // compose call function * function * return function
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                              << "Compose: " << f5->str() << " * " << f->str()
                              << " * " << f4->str();
                          BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "         (return * function * call)");
            EdgeFunctionPtrType fPrime = f4->composeWith(f)->composeWith(f5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                              << "       = " << fPrime->str();
                          BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
            // for each jump function coming into the call, propagate to
            // return site using the composed function
            auto revLookupResult = jumpFn->reverseLookup(c, d4);
            if (revLookupResult) {
              for (auto valAndFunc : revLookupResult->get()) {
                EdgeFunctionPtrType f3 = valAndFunc.second;
                if (!f3->equal_to(allTop)) {
                  d_t d3 = valAndFunc.first;
                  d_t d5_restoredCtx = restoreContextOnReturnedFact(c, d4, d5);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                                    << "Compose: " << fPrime->str() << " * "
                                    << f3->str();
                                BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
                  propagate(d3, retSiteC, d5_restoredCtx,
                            f3->composeWith(fPrime), c, false);
                }
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
    // be propagated into callers that have an incoming edge for this
    // condition
    if (SolverConfig.followReturnsPastSeeds() && inc.empty() &&
        IDEProblem.isZeroValue(d1)) {
      const std::set<n_t> callers = ICF->getCallersOf(functionThatNeedsSummary);
      for (n_t c : callers) {
        for (n_t retSiteC : ICF->getReturnSitesOfCallAt(c)) {
          FlowFunctionPtrType retFunction =
              cachedFlowEdgeFunctions.getRetFlowFunction(
                  c, functionThatNeedsSummary, n, retSiteC);
          INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          const container_type targets = computeReturnFlowFunction(
              retFunction, d1, d2, c, Container{ZeroValue});
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, retSiteC, d2, targets, true);
          for (d_t d5 : targets) {
            EdgeFunctionPtrType f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, ICF->getFunctionOf(n), n, d2, retSiteC, d5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "Queried Return Edge Function: " << f5->str());
            if (SolverConfig.emitESG()) {
              intermediateEdgeFunctions[std::make_tuple(n, d2, retSiteC, d5)]
                  .push_back(f5);
            }
            INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                              << "Compose: " << f5->str() << " * " << f->str();
                          BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
            propagteUnbalancedReturnFlow(retSiteC, d5, f->composeWith(f5), c);
            // register for value processing (2nd IDE phase)
            unbalancedRetSites.insert(retSiteC);
          }
        }
      }
      // in cases where there are no callers, the return statement would
      // normally not be processed at all; this might be undesirable if
      // the flow function has a side effect such as registering a taint;
      // instead we thus call the return flow function will a null caller
      if (callers.empty()) {
        FlowFunctionPtrType retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                nullptr, functionThatNeedsSummary, n, nullptr);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        retFunction->computeTargets(d2);
      }
    }
  }

  void propagteUnbalancedReturnFlow(n_t retSiteC, d_t targetVal,
                                    EdgeFunctionPtrType edgeFunction,
                                    n_t relatedCallSite) {
    propagate(ZeroValue, retSiteC, targetVal, std::move(edgeFunction),
              relatedCallSite, true);
  }

  /**
   * This method will be called for each incoming edge and can be used to
   * transfer knowledge from the calling edge to the returning edge, without
   * affecting the summary edges at the callee.
   * @param callSite
   *
   * @param d4
   *            Fact stored with the incoming edge, i.e., present at the
   *            caller side
   * @param d5
   *            Fact that originally should be propagated to the caller.
   * @return Fact that will be propagated to the caller.
   */
  d_t restoreContextOnReturnedFact(n_t callSite, d_t d4, d_t d5) {
    // TODO support LinkedNode and JoinHandlingNode
    //		if (d5 instanceof LinkedNode) {
    //			((LinkedNode<D>) d5).setCallingContext(d4);
    //		}
    //		if(d5 instanceof JoinHandlingNode) {
    //			((JoinHandlingNode<D>)
    // d5).setCallingContext(d4);
    //		}
    return d5;
  }

  /**
   * Computes the normal flow function for the given set of start and end
   * abstractions-
   * @param flowFunction The normal flow function to compute
   * @param d1 The abstraction at the method's start node
   * @param d2 The abstraction at the current node
   * @return The set of abstractions at the successor node
   */
  container_type
  computeNormalFlowFunction(const FlowFunctionPtrType &flowFunction, d_t d1,
                            d_t d2) {
    return flowFunction->computeTargets(d2);
  }

  /**
   * TODO: comment
   */
  container_type
  computeSummaryFlowFunction(const FlowFunctionPtrType &SummaryFlowFunction,
                             d_t d1, d_t d2) {
    return SummaryFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the call flow function for the given call-site abstraction
   * @param callFlowFunction The call flow function to compute
   * @param d1 The abstraction at the current method's start node.
   * @param d2 The abstraction at the call site
   * @return The set of caller-side abstractions at the callee's start node
   */
  container_type
  computeCallFlowFunction(const FlowFunctionPtrType &callFlowFunction, d_t d1,
                          d_t d2) {
    return callFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the call-to-return flow function for the given call-site
   * abstraction
   * @param callToReturnFlowFunction The call-to-return flow function to
   * compute
   * @param d1 The abstraction at the current method's start node.
   * @param d2 The abstraction at the call site
   * @return The set of caller-side abstractions at the return site
   */
  container_type computeCallToReturnFlowFunction(
      const FlowFunctionPtrType &callToReturnFlowFunction, d_t d1, d_t d2) {
    return callToReturnFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the return flow function for the given set of caller-side
   * abstractions.
   * @param retFunction The return flow function to compute
   * @param d1 The abstraction at the beginning of the callee
   * @param d2 The abstraction at the exit node in the callee
   * @param callSite The call site
   * @param callerSideDs The abstractions at the call site
   * @return The set of caller-side abstractions at the return site
   */
  container_type
  computeReturnFlowFunction(const FlowFunctionPtrType &retFunction, d_t d1,
                            d_t d2, n_t callSite,
                            const Container &callerSideDs) {
    return retFunction->computeTargets(d2);
  }

  /**
   * Propagates the flow further down the exploded super graph, merging any
   * edge function that might already have been computed for targetVal at
   * target.
   *
   * @param sourceVal the source value of the propagated summary edge
   * @param target the target statement
   * @param targetVal the target value at the target statement
   * @param f the new edge function computed from (s0,sourceVal) to
   * (target,targetVal)
   * @param relatedCallSite for call and return flows the related call
   * statement, nullptr otherwise (this value is not used within this
   * implementation but may be useful for subclasses of IDESolver)
   * @param isUnbalancedReturn true if this edge is propagating an
   * unbalanced return (this value is not used within this implementation
   * but may be useful for subclasses of {@link IDESolver})
   */
  void
  propagate(d_t sourceVal, n_t target, d_t targetVal,
            const EdgeFunctionPtrType &f,
            /* deliberately exposed to clients */ n_t relatedCallSite,
            /* deliberately exposed to clients */ bool isUnbalancedReturn) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "Propagate flow";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Source value  : " << IDEProblem.DtoString(sourceVal);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Target        : " << IDEProblem.NtoString(target);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Target value  : " << IDEProblem.DtoString(targetVal);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Edge function : " << f.get()->str()
                  << " (result of previous compose)";
                  BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');

    EdgeFunctionPtrType jumpFnE = [&]() {
      const auto revLookupResult = jumpFn->reverseLookup(target, targetVal);
      if (revLookupResult) {
        const auto &JumpFnContainer = revLookupResult->get();
        const auto Find = std::find_if(
            JumpFnContainer.begin(), JumpFnContainer.end(),
            [sourceVal](auto &KVpair) { return KVpair.first == sourceVal; });
        if (Find != JumpFnContainer.end()) {
          return Find->second;
        }
      }
      // jump function is initialized to all-top if no entry was found
      return allTop;
    }();
    EdgeFunctionPtrType fPrime = jumpFnE->joinWith(f);
    bool newFunction = !(fPrime->equal_to(jumpFnE));

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Join: " << jumpFnE->str() << " & " << f.get()->str()
                      << (jumpFnE->equal_to(f) ? " (EF's are equal)" : " ");
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "    = " << fPrime->str()
                  << (newFunction ? " (new jump func)" : " ");
                  BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    if (newFunction) {
      jumpFn->addFunction(sourceVal, target, targetVal, fPrime);
      const PathEdge<n_t, d_t> edge(sourceVal, target, targetVal);
      PathEdgeCount++;
      pathEdgeProcessingTask(edge);

      LOG_IF_ENABLE(if (!IDEProblem.isZeroValue(targetVal)) {
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "EDGE: <F: " << target->getFunction()->getName().str()
            << ", D: " << IDEProblem.DtoString(sourceVal) << '>';
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << " ---> <N: " << IDEProblem.NtoString(target) << ',';
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "       D: " << IDEProblem.DtoString(targetVal) << ',';
        BOOST_LOG_SEV(lg::get(), DEBUG) << "      EF: " << fPrime->str() << '>';
        BOOST_LOG_SEV(lg::get(), DEBUG) << ' ';
      });
    } else {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "PROPAGATE: No new function!");
    }
  }

  l_t joinValueAt(n_t unit, d_t fact, l_t curr, l_t newVal) {
    return IDEProblem.join(std::move(curr), std::move(newVal));
  }

  std::set<typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell>
  endSummary(n_t sP, d_t d3) {
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      auto key = std::make_pair(sP, d3);
      auto findND = fSummaryReuse.find(key);
      if (findND == fSummaryReuse.end()) {
        fSummaryReuse.emplace(key, 0);
      } else {
        fSummaryReuse[key] += 1;
      }
    }
    return endsummarytab.get(sP, d3).cellSet();
  }

  std::map<n_t, container_type> incoming(d_t d1, n_t sP) {
    return incomingtab.get(sP, d1);
  }

  void addIncoming(n_t sP, d_t d3, n_t n, d_t d2) {
    incomingtab.get(sP, d3)[n].insert(d2);
  }

  void printIncomingTab() const {
#ifdef DYNAMIC_LOG
    if (boost::log::core::get()->get_logging_enabled()) {
      BOOST_LOG_SEV(lg::get(), DEBUG) << "Start of incomingtab entry";
      for (auto cell : incomingtab.cellSet()) {
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "sP: " << IDEProblem.NtoString(cell.getRowKey());
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "d3: " << IDEProblem.DtoString(cell.getColumnKey());
        for (auto entry : cell.getValue()) {
          BOOST_LOG_SEV(lg::get(), DEBUG)
              << "  n: " << IDEProblem.NtoString(entry.first);
          for (auto fact : entry.second) {
            BOOST_LOG_SEV(lg::get(), DEBUG)
                << "  d2: " << IDEProblem.DtoString(fact);
          }
        }
        BOOST_LOG_SEV(lg::get(), DEBUG) << "---------------";
      }
      BOOST_LOG_SEV(lg::get(), DEBUG) << "End of incomingtab entry";
      BOOST_LOG_SEV(lg::get(), DEBUG) << ' ';
    }
#endif
  }

  void printEndSummaryTab() const {
#ifdef DYNAMIC_LOG
    if (boost::log::core::get()->get_logging_enabled()) {
      BOOST_LOG_SEV(lg::get(), DEBUG) << "Start of endsummarytab entry";
      for (auto cell : endsummarytab.cellVec()) {
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "sP: " << IDEProblem.NtoString(cell.getRowKey());
        BOOST_LOG_SEV(lg::get(), DEBUG)
            << "d1: " << IDEProblem.DtoString(cell.getColumnKey());
        for (auto inner_cell : cell.getValue().cellVec()) {
          BOOST_LOG_SEV(lg::get(), DEBUG)
              << "  eP: " << IDEProblem.NtoString(inner_cell.getRowKey());
          BOOST_LOG_SEV(lg::get(), DEBUG)
              << "  d2: " << IDEProblem.DtoString(inner_cell.getColumnKey());
          BOOST_LOG_SEV(lg::get(), DEBUG)
              << "  EF: " << inner_cell.getValue()->str();
          BOOST_LOG_SEV(lg::get(), DEBUG) << ' ';
        }
        BOOST_LOG_SEV(lg::get(), DEBUG) << "---------------";
        BOOST_LOG_SEV(lg::get(), DEBUG) << ' ';
      }
      BOOST_LOG_SEV(lg::get(), DEBUG) << "End of endsummarytab entry";
      BOOST_LOG_SEV(lg::get(), DEBUG) << ' ';
    }
#endif
  }

  void printComputedPathEdges() {
    std::cout << "\n**********************************************************";
    std::cout << "\n*          Computed intra-procedural path egdes          *";
    std::cout
        << "\n**********************************************************\n";

    // Sort intra-procedural path edges
    auto cells = computedIntraPathEdges.cellVec();
    StmtLess stmtless(ICF);
    sort(cells.begin(), cells.end(), [&stmtless](auto a, auto b) {
      return stmtless(a.getRowKey(), b.getRowKey());
    });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.getRowKey(), cell.getColumnKey());
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      std::cout << "\nN1: " << IDEProblem.NtoString(Edge.first) << '\n'
                << "N2: " << n2_label << "\n----"
                << std::string(n2_label.size(), '-') << '\n';
      for (auto D1ToD2Set : cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        std::cout << "D1: " << IDEProblem.DtoString(D1Fact) << '\n';
        for (auto D2Fact : D1ToD2Set.second) {
          std::cout << "\tD2: " << IDEProblem.DtoString(D2Fact) << '\n';
        }
        std::cout << '\n';
      }
    }

    std::cout << "\n**********************************************************";
    std::cout << "\n*          Computed inter-procedural path edges          *";
    std::cout
        << "\n**********************************************************\n";

    // Sort intra-procedural path edges
    cells = computedInterPathEdges.cellVec();
    sort(cells.begin(), cells.end(), [&stmtless](auto a, auto b) {
      return stmtless(a.getRowKey(), b.getRowKey());
    });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.getRowKey(), cell.getColumnKey());
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      std::cout << "\nN1: " << IDEProblem.NtoString(Edge.first) << '\n'
                << "N2: " << n2_label << "\n----"
                << std::string(n2_label.size(), '-') << '\n';
      for (auto D1ToD2Set : cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        std::cout << "D1: " << IDEProblem.DtoString(D1Fact) << '\n';
        for (auto D2Fact : D1ToD2Set.second) {
          std::cout << "\tD2: " << IDEProblem.DtoString(D2Fact) << '\n';
        }
        std::cout << '\n';
      }
    }
  }

  /**
   * The invariant for computing the number of generated (#gen) and killed
   * (#kill) facts:
   *   (1) #Valid facts at the last statement <= #gen - #kill
   *   (2) #gen >= #kill
   *
   * The total number of valid facts can be smaller than the difference of
   * generated and killed facts, due to set semantics, i.e. a fact can be
   * generated multiple times but appears only once.
   *
   * Zero value is not counted!
   *
   * @brief Computes and prints statistics of the analysis run, e.g. number of
   * generated/killed facts, number of summary-reuses etc.
   */
  void computeAndPrintStatistics() {
    PAMM_GET_INSTANCE;
    // Stores all valid facts at return site in caller context; return-site is
    // key
    std::unordered_map<n_t, std::set<d_t>> ValidInCallerContext;
    std::size_t genFacts = 0, killFacts = 0, intraPathEdges = 0,
                interPathEdges = 0;
    /* --- Intra-procedural Path Edges ---
     * d1 --> d2-Set
     * Case 1: d1 in d2-Set
     * Case 2: d1 not in d2-Set, i.e. d1 was killed. d2-Set could be empty.
     */
    for (auto cell : computedIntraPathEdges.cellSet()) {
      auto Edge = std::make_pair(cell.getRowKey(), cell.getColumnKey());
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "N1: " << IDEProblem.NtoString(Edge.first);
                    BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "N2: " << IDEProblem.NtoString(Edge.second));
      for (auto D1ToD2Set : cell.getValue()) {
        auto D1 = D1ToD2Set.first;
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "d1: " << IDEProblem.DtoString(D1));
        auto D2Set = D1ToD2Set.second;
        intraPathEdges += D2Set.size();
        // Case 1
        if (D2Set.find(D1) != D2Set.end()) {
          genFacts += D2Set.size() - 1;
        }
        // Case 2
        else {
          genFacts += D2Set.size();
          // We ignore the zero value
          if (!IDEProblem.isZeroValue(D1)) {
            killFacts++;
          }
        }
        // Store all valid facts after call-to-return flow
        if (ICF->isCallStmt(Edge.first)) {
          ValidInCallerContext[Edge.second].insert(D2Set.begin(), D2Set.end());
        }
        LOG_IF_ENABLE([&]() {
          for (auto D2 : D2Set) {
            BOOST_LOG_SEV(lg::get(), DEBUG)
                << "d2: " << IDEProblem.DtoString(D2);
          }
          BOOST_LOG_SEV(lg::get(), DEBUG) << "----";
        }());
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << " ");
    }

    // Stores all pairs of (Startpoint, Fact) for which a summary was applied
    std::set<std::pair<n_t, d_t>> ProcessSummaryFacts;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "==============================================";
                  BOOST_LOG_SEV(lg::get(), DEBUG) << "INTER PATH EDGES");
    for (auto cell : computedInterPathEdges.cellSet()) {
      auto Edge = std::make_pair(cell.getRowKey(), cell.getColumnKey());
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "N1: " << IDEProblem.NtoString(Edge.first);
                    BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "N2: " << IDEProblem.NtoString(Edge.second));
      /* --- Call-flow Path Edges ---
       * Case 1: d1 --> empty set
       *   Can be ignored, since killing a fact in the caller context will
       *   actually happen during  call-to-return.
       *
       * Case 2: d1 --> d2-Set
       *   Every fact d_i != ZeroValue in d2-set will be generated in the
       * callee context, thus counts as a new fact. Even if d1 is passed as it
       * is, it will count as a new fact. The reason for this is, that d1 can
       * be killed in the callee context, but still be valid in the caller
       * context.
       *
       * Special Case: Summary was applied for a particular call
       *   Process the summary's #gen and #kill.
       */
      if (ICF->isCallStmt(Edge.first)) {
        for (auto D1ToD2Set : cell.getValue()) {
          auto D1 = D1ToD2Set.first;
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "d1: " << IDEProblem.DtoString(D1));
          auto DSet = D1ToD2Set.second;
          interPathEdges += DSet.size();
          for (auto D2 : DSet) {
            if (!IDEProblem.isZeroValue(D2)) {
              genFacts++;
            }
            // Special case
            if (ProcessSummaryFacts.find(std::make_pair(Edge.second, D2)) !=
                ProcessSummaryFacts.end()) {
              std::multiset<d_t> SummaryDMultiSet =
                  endsummarytab.get(Edge.second, D2).columnKeySet();
              // remove duplicates from multiset
              std::set<d_t> SummaryDSet(SummaryDMultiSet.begin(),
                                        SummaryDMultiSet.end());
              // Process summary just as an intra-procedural edge
              if (SummaryDSet.find(D2) != SummaryDSet.end()) {
                genFacts += SummaryDSet.size() - 1;
              } else {
                genFacts += SummaryDSet.size();
                // We ignore the zero value
                if (!IDEProblem.isZeroValue(D1)) {
                  killFacts++;
                }
              }
            } else {
              ProcessSummaryFacts.emplace(Edge.second, D2);
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "d2: " << IDEProblem.DtoString(D2));
          }
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "----");
        }
      }
      /* --- Return-flow Path Edges ---
       * Since every fact passed to the callee was counted as a new fact, we
       * have to count every fact propagated to the caller as a kill to
       * satisfy our invariant. Obviously, every fact not propagated to the
       * caller will count as a kill. If an actual new fact is propagated to
       * the caller, we have to increase the number of generated facts by one.
       * Zero value does not count towards generated/killed facts.
       */
      if (ICF->isExitStmt(cell.getRowKey())) {
        for (auto D1ToD2Set : cell.getValue()) {
          auto D1 = D1ToD2Set.first;
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "d1: " << IDEProblem.DtoString(D1));
          auto DSet = D1ToD2Set.second;
          interPathEdges += DSet.size();
          auto CallerFacts = ValidInCallerContext[Edge.second];
          for (auto D2 : DSet) {
            // d2 not valid in caller context
            if (CallerFacts.find(D2) == CallerFacts.end()) {
              genFacts++;
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "d2: " << IDEProblem.DtoString(D2));
          }
          if (!IDEProblem.isZeroValue(D1)) {
            killFacts++;
          }
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "----");
        }
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << " ");
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "SUMMARY REUSE");
    std::size_t TotalSummaryReuse = 0;
    for (auto entry : fSummaryReuse) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "N1: " << IDEProblem.NtoString(entry.first.first);
                    BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "D1: " << IDEProblem.DtoString(entry.first.second);
                    BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "#Reuse: " << entry.second);
      TotalSummaryReuse += entry.second;
    }

    INC_COUNTER("Gen facts", genFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Kill facts", killFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Summary-reuse", TotalSummaryReuse, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Intra Path Edges", intraPathEdges, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Inter Path Edges", interPathEdges, PAMM_SEVERITY_LEVEL::Core);

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                      << "----------------------------------------------";
                  BOOST_LOG_SEV(lg::get(), INFO) << "=== Solver Statistics ===";
                  BOOST_LOG_SEV(lg::get(), INFO)
                  << "#Facts generated : " << GET_COUNTER("Gen facts");
                  BOOST_LOG_SEV(lg::get(), INFO)
                  << "#Facts killed    : " << GET_COUNTER("Kill facts");
                  BOOST_LOG_SEV(lg::get(), INFO)
                  << "#Summary-reuse   : " << GET_COUNTER("Summary-reuse");
                  BOOST_LOG_SEV(lg::get(), INFO)
                  << "#Intra Path Edges: " << GET_COUNTER("Intra Path Edges");
                  BOOST_LOG_SEV(lg::get(), INFO)
                  << "#Inter Path Edges: " << GET_COUNTER("Inter Path Edges"));
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
              << "Flow function query count: " << GET_COUNTER("FF Queries");
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Edge function query count: " << GET_COUNTER("EF Queries");
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Data-flow value propagation count: "
          << GET_COUNTER("Value Propagation");
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Data-flow value computation count: "
          << GET_COUNTER("Value Computation");
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Special flow function usage count: "
          << GET_COUNTER("SpecialSummary-FF Application");
          BOOST_LOG_SEV(lg::get(), INFO) << "Jump function construciton count: "
                                         << GET_COUNTER("JumpFn Construction");
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Phase I duration: " << PRINT_TIMER("DFA Phase I");
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Phase II duration: " << PRINT_TIMER("DFA Phase II");
          BOOST_LOG_SEV(lg::get(), INFO)
          << "----------------------------------------------");
      cachedFlowEdgeFunctions.print();
    }
  }

public:
  void enableESGAsDot() { SolverConfig.setEmitESG(); }

  void
  emitESGAsDot(std::ostream &OS = std::cout,
               std::string DotConfigDir = PhasarConfig::PhasarDirectory()) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Emit Exploded super-graph (ESG) as DOT graph";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process intra-procedural path egdes";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "=============================================");
    DOTGraph<d_t> G;
    DOTConfig::importDOTConfig(std::move(DotConfigDir));
    DOTFunctionSubGraph *FG = nullptr;

    // Sort intra-procedural path edges
    auto cells = computedIntraPathEdges.cellVec();
    StmtLess stmtless(ICF);
    sort(cells.begin(), cells.end(), [&stmtless](auto a, auto b) {
      return stmtless(a.getRowKey(), b.getRowKey());
    });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.getRowKey(), cell.getColumnKey());
      std::string n1_label = IDEProblem.NtoString(Edge.first);
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "N1: " << n1_label;
                    BOOST_LOG_SEV(lg::get(), DEBUG) << "N2: " << n2_label);
      std::string n1_stmtId = ICF->getStatementId(Edge.first);
      std::string n2_stmtId = ICF->getStatementId(Edge.second);
      std::string fnName = ICF->getFunctionOf(Edge.first)->getName().str();
      // Get or create function subgraph
      if (!FG || FG->id != fnName) {
        FG = &G.functions[fnName];
        FG->id = fnName;
      }

      // Create control flow nodes
      DOTNode N1(fnName, n1_label, n1_stmtId);
      DOTNode N2(fnName, n2_label, n2_stmtId);
      // Add control flow node(s) to function subgraph
      FG->stmts.insert(N1);
      if (ICF->isExitStmt(Edge.second)) {
        FG->stmts.insert(N2);
      }

      // Set control flow edge
      FG->intraCFEdges.emplace(N1, N2);

      DOTFactSubGraph *D1_FSG = nullptr;
      unsigned D1FactId = 0;
      unsigned D2FactId = 0;
      for (auto D1ToD2Set : cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "d1: " << IDEProblem.DtoString(D1Fact));

        DOTNode D1;
        if (IDEProblem.isZeroValue(D1Fact)) {
          D1 = {fnName, "Λ", n1_stmtId, 0, false, true};
          D1FactId = 0;
        } else {
          // Get the fact-ID
          D1FactId = G.getFactID(D1Fact);
          std::string d1_label = IDEProblem.DtoString(D1Fact);

          // Get or create the fact subgraph
          D1_FSG = FG->getOrCreateFactSG(D1FactId, d1_label);

          // Insert D1 to fact subgraph
          D1 = {fnName, d1_label, n1_stmtId, D1FactId, false, true};
          D1_FSG->nodes.insert(std::make_pair(n1_stmtId, D1));
        }

        DOTFactSubGraph *D2_FSG = nullptr;
        for (auto D2Fact : D1ToD2Set.second) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "d2: " << IDEProblem.DtoString(D2Fact));
          // We do not need to generate any intra-procedural nodes and edges
          // for the zero value since they will be auto-generated
          if (!IDEProblem.isZeroValue(D2Fact)) {
            // Get the fact-ID
            D2FactId = G.getFactID(D2Fact);
            std::string d2_label = IDEProblem.DtoString(D2Fact);
            DOTNode D2 = {fnName, d2_label, n2_stmtId, D2FactId, false, true};
            std::string EFLabel;
            auto EFVec = intermediateEdgeFunctions[std::make_tuple(
                Edge.first, D1Fact, Edge.second, D2Fact)];
            for (auto EF : EFVec) {
              EFLabel += EF->str() + ", ";
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "EF LABEL: " << EFLabel);
            if (D1FactId == D2FactId && !IDEProblem.isZeroValue(D1Fact)) {
              D1_FSG->nodes.insert(std::make_pair(n2_stmtId, D2));
              D1_FSG->edges.emplace(D1, D2, true, EFLabel);
            } else {
              // Get or create the fact subgraph
              D2_FSG = FG->getOrCreateFactSG(D2FactId, d2_label);

              D2_FSG->nodes.insert(std::make_pair(n2_stmtId, D2));
              FG->crossFactEdges.emplace(D1, D2, true, EFLabel);
            }
          }
        }
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "----------");
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << " ");
    }

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "=============================================";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Process inter-procedural path edges";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "=============================================");
    cells = computedInterPathEdges.cellVec();
    sort(cells.begin(), cells.end(), [&stmtless](auto a, auto b) {
      return stmtless(a.getRowKey(), b.getRowKey());
    });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.getRowKey(), cell.getColumnKey());
      std::string n1_label = IDEProblem.NtoString(Edge.first);
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      std::string fNameOfN1 = ICF->getFunctionOf(Edge.first)->getName().str();
      std::string fNameOfN2 = ICF->getFunctionOf(Edge.second)->getName().str();
      std::string n1_stmtId = ICF->getStatementId(Edge.first);
      std::string n2_stmtId = ICF->getStatementId(Edge.second);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "N1: " << n1_label;
                    BOOST_LOG_SEV(lg::get(), DEBUG) << "N2: " << n2_label);

      // Add inter-procedural control flow edge
      DOTNode N1(fNameOfN1, n1_label, n1_stmtId);
      DOTNode N2(fNameOfN2, n2_label, n2_stmtId);

      // Handle recursion control flow as intra-procedural control flow
      // since those eges never leave the function subgraph
      FG = nullptr;
      if (fNameOfN1 == fNameOfN2) {
        // This function subgraph is guaranteed to exist
        FG = &G.functions[fNameOfN1];
        FG->intraCFEdges.emplace(N1, N2);
      } else {
        // Check the case where the callee is a single statement function,
        // thus does not contain intra-procedural path edges. We have to
        // generate the function sub graph here!
        if (!G.functions.count(fNameOfN1)) {
          FG = &G.functions[fNameOfN1];
          FG->id = fNameOfN1;
          FG->stmts.insert(N1);
        } else if (!G.functions.count(fNameOfN2)) {
          FG = &G.functions[fNameOfN2];
          FG->id = fNameOfN2;
          FG->stmts.insert(N2);
        }
        G.interCFEdges.emplace(N1, N2);
      }

      // Create D1 and D2, if D1 == D2 == lambda then add Edge(D1, D2) to
      // interLambdaEges otherwise add Edge(D1, D2) to interFactEdges
      unsigned D1FactId = 0;
      unsigned D2FactId = 0;
      for (auto D1ToD2Set : cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "d1: " << IDEProblem.DtoString(D1Fact));
        DOTNode D1;
        if (IDEProblem.isZeroValue(D1Fact)) {
          D1 = {fNameOfN1, "Λ", n1_stmtId, 0, false, true};
        } else {
          // Get the fact-ID
          D1FactId = G.getFactID(D1Fact);
          std::string d1_label = IDEProblem.DtoString(D1Fact);
          D1 = {fNameOfN1, d1_label, n1_stmtId, D1FactId, false, true};
          // FG should already exist even for single statement functions
          if (!G.containsFactSG(fNameOfN1, D1FactId)) {
            FG = &G.functions[fNameOfN1];
            auto *D1_FSG = FG->getOrCreateFactSG(D1FactId, d1_label);
            D1_FSG->nodes.insert(std::make_pair(n1_stmtId, D1));
          }
        }

        auto D2Set = D1ToD2Set.second;
        for (auto D2Fact : D2Set) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                        << "d2: " << IDEProblem.DtoString(D2Fact));
          DOTNode D2;
          if (IDEProblem.isZeroValue(D2Fact)) {
            D2 = {fNameOfN2, "Λ", n2_stmtId, 0, false, true};
          } else {
            // Get the fact-ID
            D2FactId = G.getFactID(D2Fact);
            std::string d2_label = IDEProblem.DtoString(D2Fact);
            D2 = {fNameOfN2, d2_label, n2_stmtId, D2FactId, false, true};
            // FG should already exist even for single statement functions
            if (!G.containsFactSG(fNameOfN2, D2FactId)) {
              FG = &G.functions[fNameOfN2];
              auto *D2_FSG = FG->getOrCreateFactSG(D2FactId, d2_label);
              D2_FSG->nodes.insert(std::make_pair(n2_stmtId, D2));
            }
          }

          if (IDEProblem.isZeroValue(D1Fact) &&
              IDEProblem.isZeroValue(D2Fact)) {
            // Do not add lambda recursion edges as inter-procedural edges
            if (D1.funcName != D2.funcName) {
              G.interLambdaEdges.emplace(D1, D2, true, "AllBottom", "BOT");
            }
          } else {
            // std::string EFLabel = EF ? EF->str() : " ";
            std::string EFLabel;
            auto EFVec = intermediateEdgeFunctions[std::make_tuple(
                Edge.first, D1Fact, Edge.second, D2Fact)];
            for (auto EF : EFVec) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                            << "Partial EF Label: " << EF->str());
              EFLabel.append(EF->str() + ", ");
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                          << "EF LABEL: " << EFLabel);
            G.interFactEdges.emplace(D1, D2, true, EFLabel);
          }
        }
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "----------");
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << " ");
    }
    OS << G;
  }

  /// @brief: Allows less-than comparison based on the statement ID.
  struct StmtLess {
    const i_t *ICF;
    stringIDLess strIDLess;
    StmtLess(const i_t *ICF) : ICF(ICF), strIDLess(stringIDLess()) {}
    bool operator()(n_t lhs, n_t rhs) {
      return strIDLess(ICF->getStatementId(lhs), ICF->getStatementId(rhs));
    }
  };
};

template <typename Problem>
IDESolver(Problem &) -> IDESolver<typename Problem::ProblemAnalysisDomain,
                                  typename Problem::container_type>;

template <typename Problem>
using IDESolver_P = IDESolver<typename Problem::ProblemAnalysisDomain,
                              typename Problem::container_type>;

} // namespace psr

#endif
