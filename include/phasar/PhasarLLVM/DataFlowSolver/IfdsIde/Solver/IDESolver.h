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

#include <nlohmann/json.hpp>

#include <boost/algorithm/string/trim.hpp>

#include <llvm/Support/raw_ostream.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowEdgeFunctionCache.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSToIDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/JoinHandlingNode.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/JumpFunctions.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/LinkedNode.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/PathEdge.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ZeroedFlowFunction.h>
#include <phasar/PhasarLLVM/Utils/DOTGraph.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/PAMMMacros.h>
#include <phasar/Utils/Table.h>

namespace psr {

// Forward declare the Transformation
template <typename N, typename D, typename F, typename T, typename V,
          typename I>
class IFDSToIDETabulationProblem;

/**
 * Solves the given IDETabulationProblem as described in the 1996 paper by
 * Sagiv, Horwitz and Reps. To solve the problem, call solve(). Results
 * can then be queried by using resultAt() and resultsAt().
 *
 * @param <N> The type of nodes in the interprocedural control-flow graph.
 * @param <D> The type of data-flow facts to be computed by the tabulation
 * problem.
 * @param <F> The type of objects used to represent functions.
 * @param <T> The type of user-defined types that the type hierarchy consists of
 * @param <V> The type of values on which points-to information are computed
 * @param <L> The type of values to be computed along flow edges.
 * @param <I> The type of inter-procedural control-flow graph being used.
 */
template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class IDESolver {
public:
  using ProblemTy = IDETabulationProblem<N, D, F, T, V, L, I>;

  IDESolver(IDETabulationProblem<N, D, F, T, V, L, I> &Problem)
      : IDEProblem(Problem), ZeroValue(Problem.getZeroValue()),
        ICF(Problem.getICFG()), SolverConfig(Problem.getIFDSIDESolverConfig()),
        cachedFlowEdgeFunctions(Problem), allTop(Problem.allTopFunction()),
        jumpFn(std::make_shared<JumpFunctions<N, D, F, T, V, L, I>>(
            allTop, IDEProblem)),
        initialSeeds(Problem.initialSeeds()) {}

  IDESolver &operator=(IDESolver &&) = delete;

  virtual ~IDESolver() = default;

  nlohmann::json getAsJson() {
    const static std::string DataFlowID = "DataFlow";
    nlohmann::json J;
    auto results = this->valtab.cellSet();
    if (results.empty()) {
      J[DataFlowID] = "EMPTY";
    } else {
      std::vector<typename Table<N, D, L>::Cell> cells;
      for (auto cell : results) {
        cells.push_back(cell);
      }
      sort(cells.begin(), cells.end(),
           [](typename Table<N, D, L>::Cell a,
              typename Table<N, D, L>::Cell b) { return a.r < b.r; });
      N curr;
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
        std::string n = IDEProblem.NtoString(cells[i].r);
        boost::algorithm::trim(n);
        std::string node =
            ICF->getFunctionName(ICF->getFunctionOf(curr)) + "::" + n;
        J[DataFlowID][node];
        std::string fact = IDEProblem.DtoString(cells[i].c);
        boost::algorithm::trim(fact);
        std::string value = IDEProblem.LtoString(cells[i].v);
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

    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "IDE solver is solving the specified problem");
    // computations starting here
    START_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    // We start our analysis and construct exploded supergraph
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "Submit initial seeds, construct exploded super graph");
    submitInitialSeeds();
    STOP_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    if (SolverConfig.computeValues) {
      START_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
      // Computing the final values for the edge functions
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg, INFO)
          << "Compute the final values according to the edge functions");
      computeValues();
      STOP_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Problem solved");
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      computeAndPrintStatistics();
    }
    if (SolverConfig.emitESG) {
      emitESGasDot();
    }
  }

  /**
   * Returns the V-type result for the given value at the given statement.
   * TOP values are never returned.
   */
  virtual L resultAt(N stmt, D value) { return valtab.get(stmt, value); }

  /**
   * Returns the resulting environment for the given statement.
   * The artificial zero value can be automatically stripped.
   * TOP values are never returned.
   */
  virtual std::unordered_map<D, L> resultsAt(N stmt, bool stripZero = false) {
    std::unordered_map<D, L> result = valtab.row(stmt);
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
      // FIXME
      // llvmValueIDLess llvmIDLess;
      // std::sort(cells.begin(), cells.end(),
      //           [&llvmIDLess](
      //               typename Table<const llvm::Instruction *, D, V>::Cell a,
      //               typename Table<const llvm::Instruction *, D, V>::Cell b)
      //               {
      //             if (!llvmIDLess(a.r, b.r) && !llvmIDLess(b.r, a.r)) {
      //               if constexpr (std::is_same<D, const llvm::Value
      //               *>::value) {
      //                 return llvmIDLess(a.c, b.c);
      //               } else {
      //                 // If D is user defined we should use the user defined
      //                 // less-than comparison
      //                 return a.c < b.c;
      //               }
      //             }
      //             return llvmIDLess(a.r, b.r);
      //           });
      N prev = N{};
      N curr = N{};
      F prevFn = F{};
      F currFn = F{};
      for (unsigned i = 0; i < cells.size(); ++i) {
        curr = cells[i].r;
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
        OS << "\tD: " << IDEProblem.DtoString(cells[i].c)
           << " | V: " << IDEProblem.LtoString(cells[i].v) << '\n';
      }
    }
    OS << '\n';
    STOP_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
  }

  void dumpAllInterPathEdges() {
    std::cout << "COMPUTED INTER PATH EDGES" << std::endl;
    auto interpe = this->computedInterPathEdges.cellSet();
    for (auto &cell : interpe) {
      std::cout << "FROM" << std::endl;
      IDEProblem.printNode(std::cout, cell.r);
      std::cout << "TO" << std::endl;
      IDEProblem.printNode(std::cout, cell.c);
      std::cout << "FACTS" << std::endl;
      for (auto &fact : cell.v) {
        std::cout << "fact" << std::endl;
        IDEProblem.printFlowFact(std::cout, fact.first);
        std::cout << "produces" << std::endl;
        for (auto &out : fact.second) {
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
      IDEProblem.printNode(std::cout, cell.r);
      std::cout << "TO" << std::endl;
      IDEProblem.printNode(std::cout, cell.c);
      std::cout << "FACTS" << std::endl;
      for (auto &fact : cell.v) {
        std::cout << "fact" << std::endl;
        IDEProblem.printFlowFact(std::cout, fact.first);
        std::cout << "produces" << std::endl;
        for (auto &out : fact.second) {
          IDEProblem.printFlowFact(std::cout, out);
        }
      }
    }
  }

  SolverResults<N, D, L> getSolverResults() {
    return SolverResults<N, D, L>(this->valtab, IDEProblem.getZeroValue());
  }

protected:
  // have a shared point to allow for a copy constructor of IDESolver
  std::unique_ptr<IFDSToIDETabulationProblem<N, D, F, T, V, I>>
      TransformedProblem;
  IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem;
  D ZeroValue;
  const I *ICF;
  const IFDSIDESolverConfig SolverConfig;
  unsigned PathEdgeCount = 0;

  FlowEdgeFunctionCache<N, D, F, T, V, L, I> cachedFlowEdgeFunctions;

  Table<N, N, std::map<D, std::set<D>>> computedIntraPathEdges;

  Table<N, N, std::map<D, std::set<D>>> computedInterPathEdges;

  std::shared_ptr<EdgeFunction<L>> allTop;

  std::shared_ptr<JumpFunctions<N, D, F, T, V, L, I>> jumpFn;

  std::map<std::tuple<N, D, N, D>,
           std::vector<std::shared_ptr<EdgeFunction<L>>>>
      intermediateEdgeFunctions;

  // stores summaries that were queried before they were computed
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<N, D, Table<N, D, std::shared_ptr<EdgeFunction<L>>>> endsummarytab;

  // edges going along calls
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<N, D, std::map<N, std::set<D>>> incomingtab;

  // stores the return sites (inside callers) to which we have unbalanced
  // returns if SolverConfig.followReturnPastSeeds is enabled
  std::set<N> unbalancedRetSites;

  std::map<N, std::set<D>> initialSeeds;

  Table<N, D, L> valtab;

  std::map<std::pair<N, D>, size_t> fSummaryReuse;

  // When transforming an IFDSTabulationProblem into an IDETabulationProblem,
  // we need to allocate dynamically, otherwise the objects lifetime runs out
  // - as a modifiable r-value reference created here that should be stored in
  // a modifiable l-value reference within the IDESolver implementation leads
  // to (massive) undefined behavior (and nightmares):
  // https://stackoverflow.com/questions/34240794/understanding-the-warning-binding-r-value-to-l-value-reference
  IDESolver(IFDSTabulationProblem<N, D, F, T, V, I> &Problem)
      : TransformedProblem(
            std::make_unique<IFDSToIDETabulationProblem<N, D, F, T, V, I>>(
                Problem)),
        IDEProblem(*TransformedProblem), ZeroValue(IDEProblem.getZeroValue()),
        ICF(IDEProblem.getICFG()),
        SolverConfig(IDEProblem.getIFDSIDESolverConfig()),
        cachedFlowEdgeFunctions(IDEProblem),
        allTop(IDEProblem.allTopFunction()),
        jumpFn(std::make_shared<JumpFunctions<N, D, F, T, V, L, I>>(
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
  virtual void processCall(PathEdge<N, D> edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Call", 1, PAMM_SEVERITY_LEVEL::Full);
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process call at target: "
                  << IDEProblem.NtoString(edge.getTarget()));
    D d1 = edge.factAtSource();
    N n = edge.getTarget(); // a call node; line 14...
    D d2 = edge.factAtTarget();
    std::shared_ptr<EdgeFunction<L>> f = jumpFunction(edge);
    std::set<N> returnSiteNs = ICF->getReturnSitesOfCallAt(n);
    std::set<F> callees = ICF->getCalleesOfCallAt(n);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Possible callees:");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << callee->getName().str());
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Possible return sites:");
    for (auto ret : returnSiteNs) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  " << IDEProblem.NtoString(ret));
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    // for each possible callee
    for (F sCalledProcN : callees) { // still line 14
      // check if a special summary for the called procedure exists
      std::shared_ptr<FlowFunction<D>> specialSum =
          cachedFlowEdgeFunctions.getSummaryFlowFunction(n, sCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (specialSum) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Found and process special summary");
        for (N returnSiteN : returnSiteNs) {
          std::set<D> res = computeSummaryFlowFunction(specialSum, d1, d2);
          INC_COUNTER("SpecialSummary-FF Application", 1,
                      PAMM_SEVERITY_LEVEL::Full);
          ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, returnSiteN, d2, res, false);
          for (D d3 : res) {
            std::shared_ptr<EdgeFunction<L>> sumEdgFnE =
                cachedFlowEdgeFunctions.getSummaryEdgeFunction(n, d2,
                                                               returnSiteN, d3);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Queried Summary Edge Function: "
                          << sumEdgFnE->str());
            INC_COUNTER("SpecialSummary-EF Queries", 1,
                        PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << sumEdgFnE->str() << " * "
                          << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
            propagate(d1, returnSiteN, d3, f->composeWith(sumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        std::shared_ptr<FlowFunction<D>> function =
            cachedFlowEdgeFunctions.getCallFlowFunction(n, sCalledProcN);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<D> res = computeCallFlowFunction(function, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        // for each callee's start point(s)
        std::set<N> startPointsOf = ICF->getStartPointsOf(sCalledProcN);
        if (startPointsOf.empty()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Start points of '" +
                               ICF->getFunctionName(sCalledProcN) +
                               "' currently not available!");
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        }
        // if startPointsOf is empty, the called function is a declaration
        for (N sP : startPointsOf) {
          saveEdges(n, sP, d2, res, true);
          // for each result node of the call-flow function
          for (D d3 : res) {
            // create initial self-loop
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Create initial self-loop with D: "
                          << IDEProblem.DtoString(d3));
            propagate(d3, sP, d3, EdgeIdentity<L>::getInstance(), n,
                      false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            addIncoming(sP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            std::set<
                typename Table<N, D, std::shared_ptr<EdgeFunction<L>>>::Cell>
                endSumm = std::set<typename Table<
                    N, D, std::shared_ptr<EdgeFunction<L>>>::Cell>(
                    endSummary(sP, d3));
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
                    cachedFlowEdgeFunctions.getRetFlowFunction(n, sCalledProcN,
                                                               eP, retSiteN);
                INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
                std::set<D> returnedFacts = computeReturnFlowFunction(
                    retFunction, d3, d4, n, std::set<D>{d2});
                ADD_TO_HISTOGRAM("Data-flow facts", returnedFacts.size(), 1,
                                 PAMM_SEVERITY_LEVEL::Full);
                saveEdges(eP, retSiteN, d4, returnedFacts, true);
                // for each target value of the function
                for (D d5 : returnedFacts) {
                  // update the caller-side summary function
                  // get call edge function
                  std::shared_ptr<EdgeFunction<L>> f4 =
                      cachedFlowEdgeFunctions.getCallEdgeFunction(
                          n, d2, sCalledProcN, d3);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "Queried Call Edge Function: " << f4->str());
                  // get return edge function
                  std::shared_ptr<EdgeFunction<L>> f5 =
                      cachedFlowEdgeFunctions.getReturnEdgeFunction(
                          n, sCalledProcN, eP, d4, retSiteN, d5);
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "Queried Return Edge Function: "
                                << f5->str());
                  if (SolverConfig.emitESG) {
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
                  D d5_restoredCtx = restoreContextOnReturnedFact(n, d2, d5);
                  // propagte the effects of the entire call
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                                << "Compose: " << fPrime->str() << " * "
                                << f->str());
                  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
      for (N returnSiteN : returnSiteNs) {
        std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction =
            cachedFlowEdgeFunctions.getCallToRetFlowFunction(n, returnSiteN,
                                                             callees);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        std::set<D> returnFacts =
            computeCallToReturnFlowFunction(callToReturnFlowFunction, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", returnFacts.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        saveEdges(n, returnSiteN, d2, returnFacts, false);
        for (D d3 : returnFacts) {
          std::shared_ptr<EdgeFunction<L>> edgeFnE =
              cachedFlowEdgeFunctions.getCallToRetEdgeFunction(
                  n, d2, returnSiteN, d3, callees);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Queried Call-to-Return Edge Function: "
                        << edgeFnE->str());
          if (SolverConfig.emitESG) {
            intermediateEdgeFunctions[std::make_tuple(n, d2, returnSiteN, d3)]
                .push_back(edgeFnE);
          }
          INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          auto fPrime = f->composeWith(edgeFnE);
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "Compose: " << edgeFnE->str() << " * " << f->str()
                        << " = " << fPrime->str());
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
  virtual void processNormalFlow(PathEdge<N, D> edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Normal", 1, PAMM_SEVERITY_LEVEL::Full);
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process normal at target: "
                  << IDEProblem.NtoString(edge.getTarget()));
    D d1 = edge.factAtSource();
    N n = edge.getTarget();
    D d2 = edge.factAtTarget();
    std::shared_ptr<EdgeFunction<L>> f = jumpFunction(edge);
    auto successorInst = ICF->getSuccsOf(n);
    for (auto fn : successorInst) {
      std::shared_ptr<FlowFunction<D>> flowFunction =
          cachedFlowEdgeFunctions.getNormalFlowFunction(n, fn);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      std::set<D> res = computeNormalFlowFunction(flowFunction, d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      saveEdges(n, fn, d2, res, false);
      for (D d3 : res) {
        std::shared_ptr<EdgeFunction<L>> g =
            cachedFlowEdgeFunctions.getNormalEdgeFunction(n, d2, fn, d3);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Queried Normal Edge Function: " << g->str());
        std::shared_ptr<EdgeFunction<L>> fprime = f->composeWith(g);
        if (SolverConfig.emitESG) {
          intermediateEdgeFunctions[std::make_tuple(n, d2, fn, d3)].push_back(
              fprime);
        }
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Compose: " << g->str() << " * " << f->str() << " = "
                      << fprime->str());
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        propagate(d1, fn, d3, fprime, nullptr, false);
      }
    }
  }

  void propagateValueAtStart(std::pair<N, D> nAndD, N n) {
    PAMM_GET_INSTANCE;
    D d = nAndD.second;
    F p = ICF->getFunctionOf(n);
    for (N c : ICF->getCallsFromWithin(p)) {
      for (auto entry : jumpFn->forwardLookup(d, c)) {
        D dPrime = entry.first;
        std::shared_ptr<EdgeFunction<L>> fPrime = entry.second;
        N sP = n;
        L value = val(sP, d);
        INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
        propagateValue(c, dPrime, fPrime->computeTarget(value));
      }
    }
  }

  void propagateValueAtCall(std::pair<N, D> nAndD, N n) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    D d = nAndD.second;
    for (F q : ICF->getCalleesOfCallAt(n)) {
      std::shared_ptr<FlowFunction<D>> callFlowFunction =
          cachedFlowEdgeFunctions.getCallFlowFunction(n, q);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      for (D dPrime : callFlowFunction->computeTargets(d)) {
        std::shared_ptr<EdgeFunction<L>> edgeFn =
            cachedFlowEdgeFunctions.getCallEdgeFunction(n, d, q, dPrime);
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Queried Call Edge Function: " << edgeFn->str());
        if (SolverConfig.emitESG) {
          for (auto sP : ICF->getStartPointsOf(q)) {
            intermediateEdgeFunctions[std::make_tuple(n, d, sP, dPrime)]
                .push_back(edgeFn);
          }
        }
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        for (N startPoint : ICF->getStartPointsOf(q)) {
          INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
          propagateValue(startPoint, dPrime, edgeFn->computeTarget(val(n, d)));
        }
      }
    }
  }

  void propagateValue(N nHashN, D nHashD, L l) {
    L valNHash = val(nHashN, nHashD);
    L lPrime = joinValueAt(nHashN, nHashD, valNHash, l);
    if (!(lPrime == valNHash)) {
      setVal(nHashN, nHashD, lPrime);
      valuePropagationTask(std::pair<N, D>(nHashN, nHashD));
    }
  }

  L val(N nHashN, D nHashD) {
    if (valtab.contains(nHashN, nHashD)) {
      return valtab.get(nHashN, nHashD);
    } else {
      // implicitly initialized to top; see line [1] of Fig. 7 in SRH96 paper
      return IDEProblem.topElement();
    }
  }

  void setVal(N nHashN, D nHashD, L l) {
    auto &lg = lg::get();
    // TOP is the implicit default value which we do not need to store.
    if (l == IDEProblem.topElement()) {
      // do not store top values
      valtab.remove(nHashN, nHashD);
    } else {
      valtab.insert(nHashN, nHashD, l);
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Function : "
                  << ICF->getFunctionOf(nHashN)->getName().str());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Inst.    : " << IDEProblem.NtoString(nHashN));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Fact     : " << IDEProblem.DtoString(nHashD));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Value    : " << IDEProblem.LtoString(l));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  }

  std::shared_ptr<EdgeFunction<L>> jumpFunction(PathEdge<N, D> edge) {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "JumpFunctions Forward-Lookup:");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "   Source D: "
                  << IDEProblem.DtoString(edge.factAtSource()));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "   Target N: " << IDEProblem.NtoString(edge.getTarget()));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "   Target D: "
                  << IDEProblem.DtoString(edge.factAtTarget()));
    if (!jumpFn->forwardLookup(edge.factAtSource(), edge.getTarget())
             .count(edge.factAtTarget())) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "  => EdgeFn: " << allTop->str());
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
      // JumpFn initialized to all-top, see line [2] in SRH96 paper
      return allTop;
    }
    auto res = jumpFn->forwardLookup(edge.factAtSource(),
                                     edge.getTarget())[edge.factAtTarget()];
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "  => EdgeFn: " << res->str());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    return res;
  }

  void addEndSummary(N sP, D d1, N eP, D d2,
                     std::shared_ptr<EdgeFunction<L>> f) {
    // note: at this point we don't need to join with a potential previous f
    // because f is a jump function, which is already properly joined
    // within propagate(..)
    endsummarytab.get(sP, d1).insert(eP, d2, f);
  }

  // should be made a callable at some point
  void pathEdgeProcessingTask(PathEdge<N, D> edge) {
    PAMM_GET_INSTANCE;
    auto &lg = lg::get();
    INC_COUNTER("JumpFn Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(
        BOOST_LOG_SEV(lg, DEBUG)
        << "-------------------------------------------- " << PathEdgeCount
        << ". Path Edge --------------------------------------------");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process " << PathEdgeCount << ". path edge:");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "< D source: " << IDEProblem.DtoString(edge.factAtSource())
                  << " ;");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "  N target: " << IDEProblem.NtoString(edge.getTarget())
                  << " ;");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "  D target: " << IDEProblem.DtoString(edge.factAtTarget())
                  << " >");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    bool isCall = ICF->isCallStmt(edge.getTarget());

    if (!isCall) {
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
  void valuePropagationTask(std::pair<N, D> nAndD) {
    N n = nAndD.first;
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
  void valueComputationTask(std::vector<N> values) {
    PAMM_GET_INSTANCE;
    for (N n : values) {
      for (N sP : ICF->getStartPointsOf(ICF->getFunctionOf(n))) {
        Table<D, D, std::shared_ptr<EdgeFunction<L>>> lookupByTarget;
        lookupByTarget = jumpFn->lookupByTarget(n);
        for (typename Table<D, D, std::shared_ptr<EdgeFunction<L>>>::Cell
                 sourceValTargetValAndFunction : lookupByTarget.cellSet()) {
          D dPrime = sourceValTargetValAndFunction.getRowKey();
          D d = sourceValTargetValAndFunction.getColumnKey();
          std::shared_ptr<EdgeFunction<L>> fPrime =
              sourceValTargetValAndFunction.getValue();
          L targetVal = val(sP, dPrime);
          setVal(n, d,
                 IDEProblem.join(val(n, d), fPrime->computeTarget(targetVal)));
          INC_COUNTER("Value Computation", 1, PAMM_SEVERITY_LEVEL::Full);
        }
      }
    }
  }

  virtual void saveEdges(N sourceNode, N sinkStmt, D sourceVal,
                         std::set<D> destVals, bool interP) {
    if (!SolverConfig.recordEdges)
      return;
    Table<N, N, std::map<D, std::set<D>>> &tgtMap =
        (interP) ? computedInterPathEdges : computedIntraPathEdges;
    tgtMap.get(sourceNode, sinkStmt)[sourceVal].insert(destVals.begin(),
                                                       destVals.end());
  }

  /**
   * Computes the final values for edge functions.
   */
  void computeValues() {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Start computing values");
    // Phase II(i)
    std::map<N, std::set<D>> allSeeds(initialSeeds);
    for (N unbalancedRetSite : unbalancedRetSites) {
      if (allSeeds[unbalancedRetSite].empty()) {
        allSeeds.insert(make_pair(unbalancedRetSite, std::set<D>({ZeroValue})));
      }
    }
    // do processing
    for (const auto &seed : allSeeds) {
      N startPoint = seed.first;
      for (D val : seed.second) {
        setVal(startPoint, val, IDEProblem.topElement());
        std::pair<N, D> superGraphNode(startPoint, val);
        valuePropagationTask(superGraphNode);
      }
    }
    // Phase II(ii)
    // we create an array of all nodes and then dispatch fractions of this
    // array to multiple threads
    std::set<N> allNonCallStartNodes = ICF->allNonCallStartNodes();
    std::vector<N> nonCallStartNodesArray(allNonCallStartNodes.size());
    size_t i = 0;
    for (N n : allNonCallStartNodes) {
      nonCallStartNodesArray[i] = n;
      i++;
    }
    valueComputationTask(nonCallStartNodesArray);
  }

  /**
   * Schedules the processing of initial seeds, initiating the analysis.
   * Clients should only call this methods if performing synchronization on
   * their own. Normally, solve() should be called instead.
   */
  void submitInitialSeeds() {
    auto &lg = lg::get();
    PAMM_GET_INSTANCE;
    for (const auto &[StartPoint, Facts] : initialSeeds) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Start point: " << IDEProblem.NtoString(StartPoint));
      for (const auto &Fact : Facts) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "\tFact: " << IDEProblem.DtoString(Fact));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        if (!IDEProblem.isZeroValue(Fact)) {
          INC_COUNTER("Gen facts", 1, PAMM_SEVERITY_LEVEL::Core);
        }
        propagate(ZeroValue, StartPoint, Fact, EdgeIdentity<L>::getInstance(),
                  nullptr, false);
      }
      jumpFn->addFunction(ZeroValue, StartPoint, ZeroValue,
                          EdgeIdentity<L>::getInstance());
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
  virtual void processExit(PathEdge<N, D> edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Exit", 1, PAMM_SEVERITY_LEVEL::Full);
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process exit at target: "
                  << IDEProblem.NtoString(edge.getTarget()));
    N n = edge.getTarget(); // an exit node; line 21...
    std::shared_ptr<EdgeFunction<L>> f = jumpFunction(edge);
    F functionThatNeedsSummary = ICF->getFunctionOf(n);
    D d1 = edge.factAtSource();
    D d2 = edge.factAtTarget();
    // for each of the method's start points, determine incoming calls
    std::set<N> startPointsOf = ICF->getStartPointsOf(functionThatNeedsSummary);
    std::map<N, std::set<D>> inc;
    for (N sP : startPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      addEndSummary(sP, d1, n, d2, f);
      for (auto entry : incoming(d1, sP)) {
        inc[entry.first] = std::set<D>{entry.second};
      }
    }
    printEndSummaryTab();
    printIncomingTab();
    // for each incoming call edge already processed
    //(see processCall(..))
    for (auto entry : inc) {
      // line 22
      N c = entry.first;
      // for each return site
      for (N retSiteC : ICF->getReturnSitesOfCallAt(c)) {
        // compute return-flow function
        std::shared_ptr<FlowFunction<D>> retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                c, functionThatNeedsSummary, n, retSiteC);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        // for each incoming-call value
        for (D d4 : entry.second) {
          std::set<D> targets =
              computeReturnFlowFunction(retFunction, d1, d2, c, entry.second);
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, retSiteC, d2, targets, true);
          // for each target value at the return site
          // line 23
          for (D d5 : targets) {
            // compute composed function
            // get call edge function
            std::shared_ptr<EdgeFunction<L>> f4 =
                cachedFlowEdgeFunctions.getCallEdgeFunction(
                    c, d4, ICF->getFunctionOf(n), d1);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Queried Call Edge Function: " << f4->str());
            // get return edge function
            std::shared_ptr<EdgeFunction<L>> f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, ICF->getFunctionOf(n), n, d2, retSiteC, d5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Queried Return Edge Function: " << f5->str());
            if (SolverConfig.emitESG) {
              for (auto sP : ICF->getStartPointsOf(ICF->getFunctionOf(n))) {
                intermediateEdgeFunctions[std::make_tuple(c, d4, sP, d1)]
                    .push_back(f4);
              }
              intermediateEdgeFunctions[std::make_tuple(n, d2, retSiteC, d5)]
                  .push_back(f5);
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
            // for each jump function coming into the call, propagate to
            // return site using the composed function
            for (auto valAndFunc : jumpFn->reverseLookup(c, d4)) {
              std::shared_ptr<EdgeFunction<L>> f3 = valAndFunc.second;
              if (!f3->equal_to(allTop)) {
                D d3 = valAndFunc.first;
                D d5_restoredCtx = restoreContextOnReturnedFact(c, d4, d5);
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                              << "Compose: " << fPrime->str() << " * "
                              << f3->str());
                LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
                propagate(d3, retSiteC, d5_restoredCtx, f3->composeWith(fPrime),
                          c, false);
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
    if (SolverConfig.followReturnsPastSeeds && inc.empty() &&
        IDEProblem.isZeroValue(d1)) {
      std::set<N> callers = ICF->getCallersOf(functionThatNeedsSummary);
      for (N c : callers) {
        for (N retSiteC : ICF->getReturnSitesOfCallAt(c)) {
          std::shared_ptr<FlowFunction<D>> retFunction =
              cachedFlowEdgeFunctions.getRetFlowFunction(
                  c, functionThatNeedsSummary, n, retSiteC);
          INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          std::set<D> targets = computeReturnFlowFunction(
              retFunction, d1, d2, c, std::set<D>{ZeroValue});
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, retSiteC, d2, targets, true);
          for (D d5 : targets) {
            std::shared_ptr<EdgeFunction<L>> f5 =
                cachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, ICF->getFunctionOf(n), n, d2, retSiteC, d5);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Queried Return Edge Function: " << f5->str());
            if (SolverConfig.emitESG) {
              intermediateEdgeFunctions[std::make_tuple(n, d2, retSiteC, d5)]
                  .push_back(f5);
            }
            INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "Compose: " << f5->str() << " * " << f->str());
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
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
        std::shared_ptr<FlowFunction<D>> retFunction =
            cachedFlowEdgeFunctions.getRetFlowFunction(
                nullptr, functionThatNeedsSummary, n, nullptr);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        retFunction->computeTargets(d2);
      }
    }
  }

  void
  propagteUnbalancedReturnFlow(N retSiteC, D targetVal,
                               std::shared_ptr<EdgeFunction<L>> edgeFunction,
                               N relatedCallSite) {
    propagate(ZeroValue, retSiteC, targetVal, edgeFunction, relatedCallSite,
              true);
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
  D restoreContextOnReturnedFact(N callSite, D d4, D d5) {
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
  std::set<D>
  computeNormalFlowFunction(std::shared_ptr<FlowFunction<D>> flowFunction, D d1,
                            D d2) {
    return flowFunction->computeTargets(d2);
  }

  /**
   * TODO: comment
   */
  std::set<D> computeSummaryFlowFunction(
      std::shared_ptr<FlowFunction<D>> SummaryFlowFunction, D d1, D d2) {
    return SummaryFlowFunction->computeTargets(d2);
  }

  /**
   * Computes the call flow function for the given call-site abstraction
   * @param callFlowFunction The call flow function to compute
   * @param d1 The abstraction at the current method's start node.
   * @param d2 The abstraction at the call site
   * @return The set of caller-side abstractions at the callee's start node
   */
  std::set<D>
  computeCallFlowFunction(std::shared_ptr<FlowFunction<D>> callFlowFunction,
                          D d1, D d2) {
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
  std::set<D> computeCallToReturnFlowFunction(
      std::shared_ptr<FlowFunction<D>> callToReturnFlowFunction, D d1, D d2) {
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
  std::set<D>
  computeReturnFlowFunction(std::shared_ptr<FlowFunction<D>> retFunction, D d1,
                            D d2, N callSite, std::set<D> callerSideDs) {
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
  propagate(D sourceVal, N target, D targetVal,
            std::shared_ptr<EdgeFunction<L>> f,
            /* deliberately exposed to clients */ N relatedCallSite,
            /* deliberately exposed to clients */ bool isUnbalancedReturn) {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Propagate flow");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Source value  : " << IDEProblem.DtoString(sourceVal));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Target        : " << IDEProblem.NtoString(target));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Target value  : " << IDEProblem.DtoString(targetVal));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Edge function : " << f.get()->str()
                  << " (result of previous compose)");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    std::shared_ptr<EdgeFunction<L>> jumpFnE = nullptr;
    std::shared_ptr<EdgeFunction<L>> fPrime;
    if (!jumpFn->reverseLookup(target, targetVal).empty()) {
      jumpFnE = jumpFn->reverseLookup(target, targetVal)[sourceVal];
    }
    if (jumpFnE == nullptr) {
      jumpFnE = allTop; // jump function is initialized to all-top
    }
    fPrime = jumpFnE->joinWith(f);
    bool newFunction = !(fPrime->equal_to(jumpFnE));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Join: " << jumpFnE->str() << " & " << f.get()->str()
                  << (jumpFnE->equal_to(f) ? " (EF's are equal)" : " "));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "    = " << fPrime->str()
                  << (newFunction ? " (new jump func)" : " "));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    if (newFunction) {
      jumpFn->addFunction(sourceVal, target, targetVal, fPrime);
      PathEdge<N, D> edge(sourceVal, target, targetVal);
      PathEdgeCount++;
      pathEdgeProcessingTask(edge);
      if (!IDEProblem.isZeroValue(targetVal)) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "EDGE: <F: " << target->getFunction()->getName().str()
                      << ", D: " << IDEProblem.DtoString(sourceVal) << '>');
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << " ---> <N: " << IDEProblem.NtoString(target) << ',');
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "       D: " << IDEProblem.DtoString(targetVal)
                      << ',');
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "      EF: " << fPrime->str() << '>');
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      }
    } else {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "PROPAGATE: No new function!");
    }
  }

  L joinValueAt(N unit, D fact, L curr, L newVal) {
    return IDEProblem.join(curr, newVal);
  }

  std::set<typename Table<N, D, std::shared_ptr<EdgeFunction<L>>>::Cell>
  endSummary(N sP, D d3) {
    if (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      auto key = std::make_pair(sP, d3);
      auto findND = fSummaryReuse.find(key);
      if (findND == fSummaryReuse.end()) {
        fSummaryReuse.insert(std::make_pair(key, 0));
      } else {
        fSummaryReuse[key] += 1;
      }
    }
    return endsummarytab.get(sP, d3).cellSet();
  }

  std::map<N, std::set<D>> incoming(D d1, N sP) {
    return incomingtab.get(sP, d1);
  }

  void addIncoming(N sP, D d3, N n, D d2) {
    incomingtab.get(sP, d3)[n].insert(d2);
  }

  void printIncomingTab() {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Start of incomingtab entry");
    for (auto cell : incomingtab.cellSet()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "sP: " << IDEProblem.NtoString(cell.r));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "d3: " << IDEProblem.DtoString(cell.c));
      for (auto entry : cell.v) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  n: " << IDEProblem.NtoString(entry.first));
        for (auto fact : entry.second) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "  d2: " << IDEProblem.DtoString(fact));
        }
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "---------------");
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "End of incomingtab entry");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  }

  void printEndSummaryTab() {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Start of endsummarytab entry");
    for (auto cell : endsummarytab.cellVec()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "sP: " << IDEProblem.NtoString(cell.r));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "d1: " << IDEProblem.DtoString(cell.c));
      for (auto inner_cell : cell.v.cellVec()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  eP: " << IDEProblem.NtoString(inner_cell.r));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  d2: " << IDEProblem.DtoString(inner_cell.c));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "  EF: " << inner_cell.v->str());
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "---------------");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "End of endsummarytab entry");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  }

  void printComputedPathEdges() {
    std::cout << "\n**********************************************************";
    std::cout << "\n*          Computed intra-procedural path egdes          *";
    std::cout
        << "\n**********************************************************\n";

    // Sort intra-procedural path edges
    auto cells = computedIntraPathEdges.cellVec();
    StmtLess stmtless(ICF);
    sort(cells.begin(), cells.end(),
         [&stmtless](auto a, auto b) { return stmtless(a.r, b.r); });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.r, cell.c);
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      std::cout << "\nN1: " << IDEProblem.NtoString(Edge.first) << '\n'
                << "N2: " << n2_label << "\n----"
                << std::string(n2_label.size(), '-') << '\n';
      for (auto D1ToD2Set : cell.v) {
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
    sort(cells.begin(), cells.end(),
         [&stmtless](auto a, auto b) { return stmtless(a.r, b.r); });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.r, cell.c);
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      std::cout << "\nN1: " << IDEProblem.NtoString(Edge.first) << '\n'
                << "N2: " << n2_label << "\n----"
                << std::string(n2_label.size(), '-') << '\n';
      for (auto D1ToD2Set : cell.v) {
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
    auto &lg = lg::get();
    PAMM_GET_INSTANCE;
    // Stores all valid facts at return site in caller context; return-site is
    // key
    std::unordered_map<N, std::set<D>> ValidInCallerContext;
    std::size_t genFacts = 0, killFacts = 0, intraPathEdges = 0,
                interPathEdges = 0;
    /* --- Intra-procedural Path Edges ---
     * d1 --> d2-Set
     * Case 1: d1 in d2-Set
     * Case 2: d1 not in d2-Set, i.e. d1 was killed. d2-Set could be empty.
     */
    for (auto cell : computedIntraPathEdges.cellSet()) {
      auto Edge = std::make_pair(cell.r, cell.c);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N1: " << IDEProblem.NtoString(Edge.first));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N2: " << IDEProblem.NtoString(Edge.second));
      for (auto D1ToD2Set : cell.v) {
        auto D1 = D1ToD2Set.first;
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
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
        for (auto D2 : D2Set) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "d2: " << IDEProblem.DtoString(D2));
        }
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----");
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    }

    // Stores all pairs of (Startpoint, Fact) for which a summary was applied
    std::set<std::pair<N, D>> ProcessSummaryFacts;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "==============================================");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "INTER PATH EDGES");
    for (auto cell : computedInterPathEdges.cellSet()) {
      auto Edge = std::make_pair(cell.r, cell.c);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N1: " << IDEProblem.NtoString(Edge.first));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
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
        for (auto D1ToD2Set : cell.v) {
          auto D1 = D1ToD2Set.first;
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
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
              std::multiset<D> SummaryDMultiSet =
                  endsummarytab.get(Edge.second, D2).columnKeySet();
              // remove duplicates from multiset
              std::set<D> SummaryDSet(SummaryDMultiSet.begin(),
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
              ProcessSummaryFacts.insert(std::make_pair(Edge.second, D2));
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "d2: " << IDEProblem.DtoString(D2));
          }
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----");
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
      if (ICF->isExitStmt(cell.r)) {
        for (auto D1ToD2Set : cell.v) {
          auto D1 = D1ToD2Set.first;
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "d1: " << IDEProblem.DtoString(D1));
          auto DSet = D1ToD2Set.second;
          interPathEdges += DSet.size();
          auto CallerFacts = ValidInCallerContext[Edge.second];
          for (auto D2 : DSet) {
            // d2 not valid in caller context
            if (CallerFacts.find(D2) == CallerFacts.end()) {
              genFacts++;
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                          << "d2: " << IDEProblem.DtoString(D2));
          }
          if (!IDEProblem.isZeroValue(D1)) {
            killFacts++;
          }
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----");
        }
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    }
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "SUMMARY REUSE");
    std::size_t TotalSummaryReuse = 0;
    for (auto entry : fSummaryReuse) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "N1: " << IDEProblem.NtoString(entry.first.first));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "D1: " << IDEProblem.DtoString(entry.first.second));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "#Reuse: " << entry.second);
      TotalSummaryReuse += entry.second;
    }

    INC_COUNTER("Gen facts", genFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Kill facts", killFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Summary-reuse", TotalSummaryReuse, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Intra Path Edges", intraPathEdges, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Inter Path Edges", interPathEdges, PAMM_SEVERITY_LEVEL::Core);

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "----------------------------------------------");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "=== Solver Statistics ===");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Facts generated : " << GET_COUNTER("Gen facts"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Facts killed    : " << GET_COUNTER("Kill facts"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Summary-reuse   : " << GET_COUNTER("Summary-reuse"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Intra Path Edges: " << GET_COUNTER("Intra Path Edges"));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                  << "#Inter Path Edges: " << GET_COUNTER("Inter Path Edges"));
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Flow function query count: "
                                            << GET_COUNTER("FF Queries"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Edge function query count: "
                                            << GET_COUNTER("EF Queries"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Data-flow value propagation count: "
                    << GET_COUNTER("Value Propagation"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Data-flow value computation count: "
                    << GET_COUNTER("Value Computation"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Special flow function usage count: "
                    << GET_COUNTER("SpecialSummary-FF Application"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Jump function construciton count: "
                    << GET_COUNTER("JumpFn Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Phase I duration: " << PRINT_TIMER("DFA Phase I"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "Phase II duration: " << PRINT_TIMER("DFA Phase II"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO)
                    << "----------------------------------------------");
      cachedFlowEdgeFunctions.print();
    }
  }

public:
  void emitESGasDot() {
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Emit Exploded super-graph (ESG) as DOT graph");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process intra-procedural path egdes");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "=============================================");
    DOTGraph<D> G;
    DOTConfig::importDOTConfig();
    DOTFunctionSubGraph *FG = nullptr;

    // Sort intra-procedural path edges
    auto cells = computedIntraPathEdges.cellVec();
    StmtLess stmtless(ICF);
    sort(cells.begin(), cells.end(),
         [&stmtless](auto a, auto b) { return stmtless(a.r, b.r); });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.r, cell.c);
      std::string n1_label = IDEProblem.NtoString(Edge.first);
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "N1: " << n1_label);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "N2: " << n2_label);
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
      for (auto D1ToD2Set : cell.v) {
        auto D1Fact = D1ToD2Set.first;
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
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
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
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
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "EF LABEL: " << EFLabel);
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
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----------");
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    }

    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "=============================================");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Process inter-procedural path edges");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "=============================================");
    cells = computedInterPathEdges.cellVec();
    sort(cells.begin(), cells.end(),
         [&stmtless](auto a, auto b) { return stmtless(a.r, b.r); });
    for (auto cell : cells) {
      auto Edge = std::make_pair(cell.r, cell.c);
      std::string n1_label = IDEProblem.NtoString(Edge.first);
      std::string n2_label = IDEProblem.NtoString(Edge.second);
      std::string fNameOfN1 = ICF->getFunctionOf(Edge.first)->getName().str();
      std::string fNameOfN2 = ICF->getFunctionOf(Edge.second)->getName().str();
      std::string n1_stmtId = ICF->getStatementId(Edge.first);
      std::string n2_stmtId = ICF->getStatementId(Edge.second);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "N1: " << n1_label);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "N2: " << n2_label);

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
      for (auto D1ToD2Set : cell.v) {
        auto D1Fact = D1ToD2Set.first;
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
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
            auto D1_FSG = FG->getOrCreateFactSG(D1FactId, d1_label);
            D1_FSG->nodes.insert(std::make_pair(n1_stmtId, D1));
          }
        }

        auto D2Set = D1ToD2Set.second;
        for (auto D2Fact : D2Set) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
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
              auto D2_FSG = FG->getOrCreateFactSG(D2FactId, d2_label);
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
              EFLabel += EF->str() + ", ";
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "EF LABEL: " << EFLabel);
            G.interFactEdges.emplace(D1, D2, true, EFLabel);
          }
        }
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "----------");
      }
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << " ");
    }

    std::ofstream dotFile("ESG.dot", std::ios::binary);
    dotFile << G;
    dotFile.close();
  }

  /// @brief: Allows less-than comparison based on the statement ID.
  struct StmtLess {
    const I *ICF;
    stringIDLess strIDLess;
    StmtLess(const I *ICF) : ICF(ICF), strIDLess(stringIDLess()) {}
    bool operator()(N lhs, N rhs) {
      return strIDLess(ICF->getStatementId(lhs), ICF->getStatementId(rhs));
    }
  };
};

template <typename Problem>
IDESolver(Problem &)
    ->IDESolver<typename Problem::n_t, typename Problem::d_t,
                typename Problem::f_t, typename Problem::t_t,
                typename Problem::v_t, typename Problem::l_t,
                typename Problem::i_t>;

template <typename Problem>
using IDESolver_P = IDESolver<typename Problem::n_t, typename Problem::d_t,
                              typename Problem::f_t, typename Problem::t_t,
                              typename Problem::v_t, typename Problem::l_t,
                              typename Problem::i_t>;

} // namespace psr

#endif
