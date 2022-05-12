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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IDESOLVER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IDESOLVER_H

#include <fstream>
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
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/InitialSeeds.h"
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

/// Solves the given IDETabulationProblem as described in the 1996 paper by
/// Sagiv, Horwitz and Reps. To solve the problem, call solve(). Results
/// can then be queried by using resultAt() and resultsAt().
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
        CachedFlowEdgeFunctions(Problem), AllTop(Problem.allTopFunction()),
        JumpFn(std::make_shared<JumpFunctions<AnalysisDomainTy, Container>>(
            AllTop, IDEProblem)),
        Seeds(Problem.initialSeeds()) {}

  IDESolver(const IDESolver &) = delete;
  IDESolver &operator=(const IDESolver &) = delete;
  IDESolver(IDESolver &&) = delete;
  IDESolver &operator=(IDESolver &&) = delete;

  virtual ~IDESolver() = default;

  nlohmann::json getAsJson() {
    using TableCell = typename Table<n_t, d_t, l_t>::Cell;
    const static std::string DataFlowID = "DataFlow";
    nlohmann::json J;
    auto Results = this->ValTab.cellSet();
    if (Results.empty()) {
      J[DataFlowID] = "EMPTY";
    } else {
      std::vector<TableCell> Cells(Results.begin(), Results.end());
      sort(Cells.begin(), Cells.end(), [](TableCell Lhs, TableCell Rhs) {
        return Lhs.getRowKey() < Rhs.getRowKey();
      });
      n_t Curr;
      for (unsigned I = 0; I < Cells.size(); ++I) {
        Curr = Cells[I].getRowKey();
        std::string NStr = IDEProblem.NtoString(Cells[I].getRowKey());
        boost::algorithm::trim(NStr);
        std::string NodeStr =
            ICF->getFunctionName(ICF->getFunctionOf(Curr)) + "::" + NStr;
        J[DataFlowID][NodeStr];
        std::string FactStr = IDEProblem.DtoString(Cells[I].getColumnKey());
        boost::algorithm::trim(FactStr);
        std::string ValueStr = IDEProblem.LtoString(Cells[I].getValue());
        boost::algorithm::trim(ValueStr);
        J[DataFlowID][NodeStr]["Facts"] += {FactStr, ValueStr};
      }
    }
    return J;
  }

  /// \brief Runs the solver on the configured problem. This can take some time.
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

    PHASAR_LOG_LEVEL(INFO, "IDE solver is solving the specified problem");
    PHASAR_LOG_LEVEL(INFO,
                     "Submit initial seeds, construct exploded super graph");
    // computations starting here
    START_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    // We start our analysis and construct exploded supergraph
    submitInitialSeeds();
    STOP_TIMER("DFA Phase I", PAMM_SEVERITY_LEVEL::Full);
    if (SolverConfig.computeValues()) {
      START_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
      // Computing the final values for the edge functions
      PHASAR_LOG_LEVEL(
          INFO, "Compute the final values according to the edge functions");
      computeValues();
      STOP_TIMER("DFA Phase II", PAMM_SEVERITY_LEVEL::Full);
    }
    PHASAR_LOG_LEVEL(INFO, "Problem solved");
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      computeAndPrintStatistics();
    }
    if (SolverConfig.emitESG()) {
      emitESGAsDot();
    }
  }

  /// Returns the L-type result for the given value at the given statement.
  [[nodiscard]] virtual l_t resultAt(n_t Stmt, d_t Value) {
    return ValTab.get(Stmt, Value);
  }

  /// Returns the L-type result at the given statement for the given data-flow
  /// fact while respecting LLVM's SSA semantics.
  ///
  /// An example: when a value is loaded and the location loaded from, here
  /// variable 'i', is a data-flow fact that holds, then the loaded value '%0'
  /// will usually be generated and also holds. However, due to the underlying
  /// theory (and respective implementation) this load instruction causes the
  /// loaded value to be generated and thus, it will be valid only AFTER the
  /// load instruction, i.e., at the successor instruction.
  ///
  ///   %0 = load i32, i32* %i, align 4
  ///
  /// This result accessor function returns the results at the successor
  /// instruction(s) reflecting that the expression on the left-hand side holds
  /// if the expression on the right-hand side holds.
  template <typename NTy = n_t>
  [[nodiscard]] typename std::enable_if_t<
      std::is_same_v<std::remove_reference_t<NTy>, llvm::Instruction *>, l_t>
  resultAtInLLVMSSA(NTy Stmt, d_t Value) {
    if (Stmt->getType()->isVoidTy()) {
      return ValTab.get(Stmt, Value);
    }
    assert(Stmt->getNextNode() && "Expected to find a valid successor node!");
    return ValTab.get(Stmt->getNextNode(), Value);
  }

  /// Returns the resulting environment for the given statement.
  /// The artificial zero value can be automatically stripped.
  /// TOP values are never returned.
  [[nodiscard]] virtual std::unordered_map<d_t, l_t>
  resultsAt(n_t Stmt, bool StripZero = false) /*TODO const*/ {
    std::unordered_map<d_t, l_t> Result = ValTab.row(Stmt);
    if (StripZero) {
      for (auto It = Result.begin(); It != Result.end();) {
        if (IDEProblem.isZeroValue(It->first)) {
          It = Result.erase(It);
        } else {
          ++It;
        }
      }
    }
    return Result;
  }

  /// Returns the data-flow results at the given statement while respecting
  /// LLVM's SSA semantics.
  ///
  /// An example: when a value is loaded and the location loaded from, here
  /// variable 'i', is a data-flow fact that holds, then the loaded value '%0'
  /// will usually be generated and also holds. However, due to the underlying
  /// theory (and respective implementation) this load instruction causes the
  /// loaded value to be generated and thus, it will be valid only AFTER the
  /// load instruction, i.e., at the successor instruction.
  ///
  ///   %0 = load i32, i32* %i, align 4
  ///
  /// This result accessor function returns the results at the successor
  /// instruction(s) reflecting that the expression on the left-hand side holds
  /// if the expression on the right-hand side holds.
  template <typename NTy = n_t>
  [[nodiscard]] typename std::enable_if_t<
      std::is_same_v<std::remove_reference_t<NTy>, llvm::Instruction *>,
      std::unordered_map<d_t, l_t>>
  resultsAtInLLVMSSA(NTy Stmt, bool StripZero = false) {
    std::unordered_map<d_t, l_t> Result = [this, Stmt]() {
      if (Stmt->getType()->isVoidTy()) {
        return ValTab.row(Stmt);
      }
      return ValTab.row(Stmt->getNextNode());
    }();
    if (StripZero) {
      // TODO: replace with std::erase_if (C++20)
      for (auto It = Result.begin(); It != Result.end();) {
        if (IDEProblem.isZeroValue(It->first)) {
          It = Result.erase(It);
        } else {
          ++It;
        }
      }
    }
    return Result;
  }

  virtual void emitTextReport(llvm::raw_ostream &OS = llvm::outs()) {
    IDEProblem.emitTextReport(getSolverResults(), OS);
  }

  virtual void emitGraphicalReport(llvm::raw_ostream &OS = llvm::outs()) {
    IDEProblem.emitGraphicalReport(getSolverResults(), OS);
  }

  virtual void dumpResults(llvm::raw_ostream &OS = llvm::outs()) {
    PAMM_GET_INSTANCE;
    START_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
    OS << "\n***************************************************************\n"
       << "*                  Raw IDESolver results                      *\n"
       << "***************************************************************\n";
    auto Cells = this->ValTab.cellVec();
    if (Cells.empty()) {
      OS << "No results computed!" << '\n';
    } else {
      LLVMValueIDLess LLVMIDLess;
      std::sort(
          Cells.begin(), Cells.end(),
          [&LLVMIDLess](const auto &Lhs, const auto &Rhs) {
            if constexpr (std::is_same_v<n_t, const llvm::Instruction *>) {
              return LLVMIDLess(Lhs.getRowKey(), Rhs.getRowKey());
            } else {
              // If non-LLVM IR is used
              return Lhs.getRowKey() < Rhs.getRowKey();
            }
          });
      n_t Prev = n_t{};
      n_t Curr = n_t{};
      f_t PrevFn = f_t{};
      f_t CurrFn = f_t{};
      for (unsigned I = 0; I < Cells.size(); ++I) {
        Curr = Cells[I].getRowKey();
        CurrFn = ICF->getFunctionOf(Curr);
        if (PrevFn != CurrFn) {
          PrevFn = CurrFn;
          OS << "\n\n============ Results for function '" +
                    ICF->getFunctionName(CurrFn) + "' ============\n";
        }
        if (Prev != Curr) {
          Prev = Curr;
          std::string NString = IDEProblem.NtoString(Curr);
          std::string Line(NString.size(), '-');
          OS << "\n\nN: " << NString << "\n---" << Line << '\n';
        }
        OS << "\tD: " << IDEProblem.DtoString(Cells[I].getColumnKey())
           << " | V: " << IDEProblem.LtoString(Cells[I].getValue()) << '\n';
      }
    }
    OS << '\n';
    STOP_TIMER("DFA IDE Result Dumping", PAMM_SEVERITY_LEVEL::Full);
  }

  void dumpAllInterPathEdges() {
    llvm::outs() << "COMPUTED INTER PATH EDGES" << '\n';
    auto Interpe = this->computedInterPathEdges.cellSet();
    for (const auto &Cell : Interpe) {
      llvm::outs() << "FROM" << '\n';
      IDEProblem.printNode(llvm::outs(), Cell.getRowKey());
      llvm::outs() << "TO" << '\n';
      IDEProblem.printNode(llvm::outs(), Cell.getColumnKey());
      llvm::outs() << "FACTS" << '\n';
      for (const auto &Fact : Cell.getValue()) {
        llvm::outs() << "fact" << '\n';
        IDEProblem.printDataFlowFact(llvm::outs(), Fact.first);
        llvm::outs() << "produces" << '\n';
        for (const auto &Out : Fact.second) {
          IDEProblem.printDataFlowFact(llvm::outs(), Out);
        }
      }
    }
  }

  void dumpAllIntraPathEdges() {
    llvm::outs() << "COMPUTED INTRA PATH EDGES" << '\n';
    auto Intrape = this->computedIntraPathEdges.cellSet();
    for (auto &Cell : Intrape) {
      llvm::outs() << "FROM" << '\n';
      IDEProblem.printNode(llvm::outs(), Cell.getRowKey());
      llvm::outs() << "TO" << '\n';
      IDEProblem.printNode(llvm::outs(), Cell.getColumnKey());
      llvm::outs() << "FACTS" << '\n';
      for (auto &Fact : Cell.getValue()) {
        llvm::outs() << "fact" << '\n';
        IDEProblem.printDataFlowFact(llvm::outs(), Fact.first);
        llvm::outs() << "produces" << '\n';
        for (auto &Out : Fact.second) {
          IDEProblem.printDataFlowFact(llvm::outs(), Out);
        }
      }
    }
  }

  SolverResults<n_t, d_t, l_t> getSolverResults() {
    return SolverResults<n_t, d_t, l_t>(this->ValTab,
                                        IDEProblem.getZeroValue());
  }

protected:
  // have a shared point to allow for a copy constructor of IDESolver
  IDETabulationProblem<AnalysisDomainTy, Container> &IDEProblem;
  d_t ZeroValue;
  const i_t *ICF;
  IFDSIDESolverConfig &SolverConfig;
  unsigned PathEdgeCount = 0;

  FlowEdgeFunctionCache<AnalysisDomainTy, Container> CachedFlowEdgeFunctions;

  Table<n_t, n_t, std::map<d_t, Container>> ComputedIntraPathEdges;

  Table<n_t, n_t, std::map<d_t, Container>> ComputedInterPathEdges;

  EdgeFunctionPtrType AllTop;

  std::shared_ptr<JumpFunctions<AnalysisDomainTy, Container>> JumpFn;

  std::map<std::tuple<n_t, d_t, n_t, d_t>, std::vector<EdgeFunctionPtrType>>
      IntermediateEdgeFunctions;

  // stores summaries that were queried before they were computed
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<n_t, d_t, Table<n_t, d_t, EdgeFunctionPtrType>> EndsummaryTab;

  // edges going along calls
  // see CC 2010 paper by Naeem, Lhotak and Rodriguez
  Table<n_t, d_t, std::map<n_t, Container>> IncomingTab;

  // stores the return sites (inside callers) to which we have unbalanced
  // returns if SolverConfig.followReturnPastSeeds is enabled
  std::set<n_t> UnbalancedRetSites;

  InitialSeeds<n_t, d_t, l_t> Seeds;

  Table<n_t, d_t, l_t> ValTab;

  std::map<std::pair<n_t, d_t>, size_t> FSummaryReuse;

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
        CachedFlowEdgeFunctions(IDEProblem),
        AllTop(IDEProblem.allTopFunction()),
        JumpFn(std::make_shared<JumpFunctions<AnalysisDomainTy, Container>>(
            AllTop, IDEProblem)),
        Seeds(IDEProblem.initialSeeds()) {}

  /// Lines 13-20 of the algorithm; processing a call site in the caller's
  /// context.
  ///
  /// For each possible callee, registers incoming call edges.
  /// Also propagates call-to-return flows and summarized callee flows within
  /// the caller.
  ///
  /// 	The following cases must be considered and handled:
  ///		1. Process as usual and just process the call
  ///		2. Create a new summary for that function (which shall be done
  ///       by the problem)
  ///		3. Just use an existing summary provided by the problem
  ///		4. If a special function is called, use a special summary
  ///       function
  ///
  /// @param edge an edge whose target node resembles a method call
  ///
  virtual void processCall(const PathEdge<n_t, d_t> Edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Call", 1, PAMM_SEVERITY_LEVEL::Full);
    PHASAR_LOG_LEVEL(DEBUG, "Process call at target: "
                                << IDEProblem.NtoString(Edge.getTarget()));
    d_t d1 = Edge.factAtSource();
    n_t n = Edge.getTarget();
    // a call node; line 14...
    d_t d2 = Edge.factAtTarget();
    EdgeFunctionPtrType f = jumpFunction(Edge);
    const std::set<n_t> ReturnSiteNs = ICF->getReturnSitesOfCallAt(n);
    const std::set<f_t> Callees = ICF->getCalleesOfCallAt(n);

    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Possible callees:"); for (auto Callee
                                                           : Callees) {
          PHASAR_LOG_LEVEL(DEBUG, "  " << Callee->getName());
        } PHASAR_LOG_LEVEL(DEBUG, "Possible return sites:");
        for (auto ret
             : ReturnSiteNs) {
          PHASAR_LOG_LEVEL(DEBUG, "  " << IDEProblem.NtoString(ret));
        });

    // for each possible callee
    for (f_t SCalledProcN : Callees) { // still line 14
      // check if a special summary for the called procedure exists
      FlowFunctionPtrType SpecialSum =
          CachedFlowEdgeFunctions.getSummaryFlowFunction(n, SCalledProcN);
      // if a special summary is available, treat this as a normal flow
      // and use the summary flow and edge functions
      if (SpecialSum) {
        PHASAR_LOG_LEVEL(DEBUG, "Found and process special summary");
        for (n_t ReturnSiteN : ReturnSiteNs) {
          container_type Res = computeSummaryFlowFunction(SpecialSum, d1, d2);
          INC_COUNTER("SpecialSummary-FF Application", 1,
                      PAMM_SEVERITY_LEVEL::Full);
          ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, ReturnSiteN, d2, Res, false);
          for (d_t d3 : Res) {
            EdgeFunctionPtrType SumEdgFnE =
                CachedFlowEdgeFunctions.getSummaryEdgeFunction(n, d2,
                                                               ReturnSiteN, d3);
            INC_COUNTER("SpecialSummary-EF Queries", 1,
                        PAMM_SEVERITY_LEVEL::Full);
            IF_LOG_ENABLED(
                PHASAR_LOG_LEVEL(DEBUG, "Queried Summary Edge Function: "
                                            << SumEdgFnE->str());
                PHASAR_LOG_LEVEL(DEBUG, "Compose: " << SumEdgFnE->str() << " * "
                                                    << f->str() << '\n'));
            propagate(d1, ReturnSiteN, d3, f->composeWith(SumEdgFnE), n, false);
          }
        }
      } else {
        // compute the call-flow function
        FlowFunctionPtrType Function =
            CachedFlowEdgeFunctions.getCallFlowFunction(n, SCalledProcN);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        container_type Res = computeCallFlowFunction(Function, d1, d2);
        ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                         PAMM_SEVERITY_LEVEL::Full);
        // for each callee's start point(s)
        std::set<n_t> StartPointsOf = ICF->getStartPointsOf(SCalledProcN);
        if (StartPointsOf.empty()) {
          PHASAR_LOG_LEVEL(DEBUG, "Start points of '" +
                                      ICF->getFunctionName(SCalledProcN) +
                                      "' currently not available!");
        }
        // if startPointsOf is empty, the called function is a declaration
        for (n_t SP : StartPointsOf) {
          saveEdges(n, SP, d2, Res, true);
          // for each result node of the call-flow function
          for (d_t d3 : Res) {
            using TableCell =
                typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell;
            // create initial self-loop
            PHASAR_LOG_LEVEL(DEBUG, "Create initial self-loop with D: "
                                        << IDEProblem.DtoString(d3));
            propagate(d3, SP, d3, EdgeIdentity<l_t>::getInstance(), n,
                      false); // line 15
            // register the fact that <sp,d3> has an incoming edge from <n,d2>
            // line 15.1 of Naeem/Lhotak/Rodriguez
            addIncoming(SP, d3, n, d2);
            // line 15.2, copy to avoid concurrent modification exceptions by
            // other threads
            // const std::set<TableCell> endSumm(endSummary(sP, d3));
            // llvm::outs() << "ENDSUMM" << '\n';
            // llvm::outs() << "Size: " << endSumm.size() << '\n';
            // llvm::outs() << "sP: " << IDEProblem.NtoString(sP)
            //           << "\nd3: " << IDEProblem.DtoString(d3)
            //           << '\n';
            // printEndSummaryTab();
            // still line 15.2 of Naeem/Lhotak/Rodriguez
            // for each already-queried exit value <eP,d4> reachable from
            // <sP,d3>, create new caller-side jump functions to the return
            // sites because we have observed a potentially new incoming
            // edge into <sP,d3>
            for (const TableCell &Entry : endSummary(SP, d3)) {
              n_t eP = Entry.getRowKey();
              d_t d4 = Entry.getColumnKey();
              EdgeFunctionPtrType fCalleeSummary = Entry.getValue();
              // for each return site
              for (n_t RetSiteN : ReturnSiteNs) {
                // compute return-flow function
                FlowFunctionPtrType RetFunction =
                    CachedFlowEdgeFunctions.getRetFlowFunction(n, SCalledProcN,
                                                               eP, RetSiteN);
                INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
                const container_type ReturnedFacts = computeReturnFlowFunction(
                    RetFunction, d3, d4, n, Container{d2});
                ADD_TO_HISTOGRAM("Data-flow facts", returnedFacts.size(), 1,
                                 PAMM_SEVERITY_LEVEL::Full);
                saveEdges(eP, RetSiteN, d4, ReturnedFacts, true);
                // for each target value of the function
                for (d_t d5 : ReturnedFacts) {
                  // update the caller-side summary function
                  // get call edge function
                  EdgeFunctionPtrType f4 =
                      CachedFlowEdgeFunctions.getCallEdgeFunction(
                          n, d2, SCalledProcN, d3);
                  PHASAR_LOG_LEVEL(DEBUG,
                                   "Queried Call Edge Function: " << f4->str());
                  // get return edge function
                  EdgeFunctionPtrType f5 =
                      CachedFlowEdgeFunctions.getReturnEdgeFunction(
                          n, SCalledProcN, eP, d4, RetSiteN, d5);
                  PHASAR_LOG_LEVEL(
                      DEBUG, "Queried Return Edge Function: " << f5->str());
                  if (SolverConfig.emitESG()) {
                    for (auto SP : ICF->getStartPointsOf(SCalledProcN)) {
                      IntermediateEdgeFunctions[std::make_tuple(n, d2, SP, d3)]
                          .push_back(f4);
                    }
                    IntermediateEdgeFunctions[std::make_tuple(eP, d4, RetSiteN,
                                                              d5)]
                        .push_back(f5);
                  }
                  INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
                  // compose call * calleeSummary * return edge functions
                  PHASAR_LOG_LEVEL(DEBUG, "Compose: " << f5->str() << " * "
                                                      << fCalleeSummary->str()
                                                      << " * " << f4->str());
                  PHASAR_LOG_LEVEL(DEBUG,
                                   "         (return * calleeSummary * call)");
                  EdgeFunctionPtrType fPrime =
                      f4->composeWith(fCalleeSummary)->composeWith(f5);
                  PHASAR_LOG_LEVEL(DEBUG, "       = " << fPrime->str());
                  d_t d5_restoredCtx = restoreContextOnReturnedFact(n, d2, d5);
                  // propagte the effects of the entire call
                  PHASAR_LOG_LEVEL(DEBUG, "Compose: " << fPrime->str() << " * "
                                                      << f->str());
                  propagate(d1, RetSiteN, d5_restoredCtx,
                            f->composeWith(fPrime), n, false);
                }
              }
            }
          }
        }
      }
    }
    // line 17-19 of Naeem/Lhotak/Rodriguez
    // process intra-procedural flows along call-to-return flow functions
    for (n_t ReturnSiteN : ReturnSiteNs) {
      FlowFunctionPtrType CallToReturnFF =
          CachedFlowEdgeFunctions.getCallToRetFlowFunction(n, ReturnSiteN,
                                                           Callees);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      container_type ReturnFacts =
          computeCallToReturnFlowFunction(CallToReturnFF, d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", returnFacts.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      saveEdges(n, ReturnSiteN, d2, ReturnFacts, false);
      for (d_t d3 : ReturnFacts) {
        EdgeFunctionPtrType EdgeFnE =
            CachedFlowEdgeFunctions.getCallToRetEdgeFunction(n, d2, ReturnSiteN,
                                                             d3, Callees);
        PHASAR_LOG_LEVEL(
            DEBUG, "Queried Call-to-Return Edge Function: " << EdgeFnE->str());
        if (SolverConfig.emitESG()) {
          IntermediateEdgeFunctions[std::make_tuple(n, d2, ReturnSiteN, d3)]
              .push_back(EdgeFnE);
        }
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        auto fPrime = f->composeWith(EdgeFnE);
        PHASAR_LOG_LEVEL(DEBUG, "Compose: " << EdgeFnE->str() << " * "
                                            << f->str() << " = "
                                            << fPrime->str());
        propagate(d1, ReturnSiteN, d3, fPrime, n, false);
      }
    }
  }

  /// Lines 33-37 of the algorithm.
  /// Simply propagate normal, intra-procedural flows.
  /// @param edge
  ///
  virtual void processNormalFlow(const PathEdge<n_t, d_t> Edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Normal", 1, PAMM_SEVERITY_LEVEL::Full);
    PHASAR_LOG_LEVEL(DEBUG, "Process normal at target: "
                                << IDEProblem.NtoString(Edge.getTarget()));
    d_t d1 = Edge.factAtSource();
    n_t n = Edge.getTarget();
    d_t d2 = Edge.factAtTarget();
    EdgeFunctionPtrType f = jumpFunction(Edge);
    for (const auto nPrime : ICF->getSuccsOf(n)) {
      FlowFunctionPtrType FlowFunc =
          CachedFlowEdgeFunctions.getNormalFlowFunction(n, nPrime);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      const container_type Res = computeNormalFlowFunction(FlowFunc, d1, d2);
      ADD_TO_HISTOGRAM("Data-flow facts", res.size(), 1,
                       PAMM_SEVERITY_LEVEL::Full);
      saveEdges(n, nPrime, d2, Res, false);
      for (d_t d3 : Res) {
        EdgeFunctionPtrType g =
            CachedFlowEdgeFunctions.getNormalEdgeFunction(n, d2, nPrime, d3);
        PHASAR_LOG_LEVEL(DEBUG, "Queried Normal Edge Function: " << g->str());
        EdgeFunctionPtrType fPrime = f->composeWith(g);
        if (SolverConfig.emitESG()) {
          IntermediateEdgeFunctions[std::make_tuple(n, d2, nPrime, d3)]
              .push_back(g);
        }
        PHASAR_LOG_LEVEL(DEBUG, "Compose: " << g->str() << " * " << f->str()
                                            << " = " << fPrime->str());
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        propagate(d1, nPrime, d3, fPrime, nullptr, false);
      }
    }
  }

  void propagateValueAtStart(const std::pair<n_t, d_t> NAndD, n_t Stmt) {
    PAMM_GET_INSTANCE;
    d_t Fact = NAndD.second;
    f_t Func = ICF->getFunctionOf(Stmt);
    for (const n_t CallSite : ICF->getCallsFromWithin(Func)) {
      auto LookupResults = JumpFn->forwardLookup(Fact, CallSite);
      if (!LookupResults) {
        continue;
      }
      for (size_t I = 0; I < LookupResults->get().size(); ++I) {
        auto Entry = LookupResults->get()[I];
        d_t dPrime = Entry.first;
        EdgeFunctionPtrType fPrime = Entry.second;
        n_t SP = Stmt;
        l_t Val = val(SP, Fact);
        INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
        propagateValue(CallSite, dPrime, fPrime->computeTarget(Val));
      }
    }
  }

  void propagateValueAtCall(const std::pair<n_t, d_t> NAndD, n_t Stmt) {
    PAMM_GET_INSTANCE;
    d_t Fact = NAndD.second;
    for (const f_t Callee : ICF->getCalleesOfCallAt(Stmt)) {
      FlowFunctionPtrType CallFlowFunction =
          CachedFlowEdgeFunctions.getCallFlowFunction(Stmt, Callee);
      INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
      for (const d_t dPrime : CallFlowFunction->computeTargets(Fact)) {
        EdgeFunctionPtrType EdgeFn =
            CachedFlowEdgeFunctions.getCallEdgeFunction(Stmt, Fact, Callee,
                                                        dPrime);
        PHASAR_LOG_LEVEL(DEBUG,
                         "Queried Call Edge Function: " << EdgeFn->str());
        if (SolverConfig.emitESG()) {
          for (const auto SP : ICF->getStartPointsOf(Callee)) {
            IntermediateEdgeFunctions[std::make_tuple(Stmt, Fact, SP, dPrime)]
                .push_back(EdgeFn);
          }
        }
        INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        for (const n_t StartPoint : ICF->getStartPointsOf(Callee)) {
          INC_COUNTER("Value Propagation", 1, PAMM_SEVERITY_LEVEL::Full);
          propagateValue(StartPoint, dPrime,
                         EdgeFn->computeTarget(val(Stmt, Fact)));
        }
      }
    }
  }

  void propagateValue(n_t NHashN, d_t NHashD, const l_t &L) {
    l_t ValNHash = val(NHashN, NHashD);
    l_t LPrime = joinValueAt(NHashN, NHashD, ValNHash, L);
    if (!(LPrime == ValNHash)) {
      setVal(NHashN, NHashD, std::move(LPrime));
      valuePropagationTask(std::pair<n_t, d_t>(NHashN, NHashD));
    }
  }

  l_t val(n_t NHashN, d_t NHashD) {
    if (ValTab.contains(NHashN, NHashD)) {
      return ValTab.get(NHashN, NHashD);
    }
    // implicitly initialized to top; see line [1] of Fig. 7 in SRH96 paper
    return IDEProblem.topElement();
  }

  void setVal(n_t NHashN, d_t NHashD, l_t L) {
    IF_LOG_ENABLED({
      PHASAR_LOG_LEVEL(DEBUG,
                       "Function : " << ICF->getFunctionOf(NHashN)->getName());
      PHASAR_LOG_LEVEL(DEBUG, "Inst.    : " << IDEProblem.NtoString(NHashN));
      PHASAR_LOG_LEVEL(DEBUG, "Fact     : " << IDEProblem.DtoString(NHashD));
      PHASAR_LOG_LEVEL(DEBUG, "Value    : " << IDEProblem.LtoString(L));
      PHASAR_LOG_LEVEL(DEBUG, ' ');
    });
    // TOP is the implicit default value which we do not need to store.
    // if (l == IDEProblem.topElement()) {
    // do not store top values
    // ValTab.remove(nHashN, nHashD);
    // } else {
    ValTab.insert(NHashN, NHashD, std::move(L));
    // }
  }

  EdgeFunctionPtrType jumpFunction(const PathEdge<n_t, d_t> Edge) {
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "JumpFunctions Forward-Lookup:");
        PHASAR_LOG_LEVEL(DEBUG, "   Source D: " << IDEProblem.DtoString(
                                    Edge.factAtSource()));
        PHASAR_LOG_LEVEL(
            DEBUG, "   Target N: " << IDEProblem.NtoString(Edge.getTarget()));
        PHASAR_LOG_LEVEL(DEBUG, "   Target D: " << IDEProblem.DtoString(
                                    Edge.factAtTarget())));

    auto FwdLookupRes =
        JumpFn->forwardLookup(Edge.factAtSource(), Edge.getTarget());
    if (FwdLookupRes) {
      auto &Ref = FwdLookupRes->get();
      if (auto Find = std::find_if(Ref.begin(), Ref.end(),
                                   [Edge](const auto &Pair) {
                                     return Edge.factAtTarget() == Pair.first;
                                   });
          Find != Ref.end()) {
        PHASAR_LOG_LEVEL(DEBUG, "  => EdgeFn: " << Find->second->str());
        return Find->second;
      }
    }
    PHASAR_LOG_LEVEL(DEBUG, "  => EdgeFn: " << AllTop->str());
    // JumpFn initialized to all-top, see line [2] in SRH96 paper
    return AllTop;
  }

  void addEndSummary(n_t SP, d_t d1, n_t eP, d_t d2, EdgeFunctionPtrType f) {
    // note: at this point we don't need to join with a potential previous f
    // because f is a jump function, which is already properly joined
    // within propagate(..)
    EndsummaryTab.get(SP, d1).insert(eP, d2, std::move(f));
  }

  // should be made a callable at some point
  void pathEdgeProcessingTask(const PathEdge<n_t, d_t> Edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("JumpFn Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(
            DEBUG,
            "-------------------------------------------- "
                << PathEdgeCount
                << ". Path Edge --------------------------------------------");
        PHASAR_LOG_LEVEL(DEBUG, ' ');
        PHASAR_LOG_LEVEL(DEBUG, "Process " << PathEdgeCount << ". path edge:");
        PHASAR_LOG_LEVEL(DEBUG, "< D source: "
                                    << IDEProblem.DtoString(Edge.factAtSource())
                                    << " ;");
        PHASAR_LOG_LEVEL(DEBUG, "  N target: "
                                    << IDEProblem.NtoString(Edge.getTarget())
                                    << " ;");
        PHASAR_LOG_LEVEL(DEBUG, "  D target: "
                                    << IDEProblem.DtoString(Edge.factAtTarget())
                                    << " >");
        PHASAR_LOG_LEVEL(DEBUG, ' '));

    if (!ICF->isCallSite(Edge.getTarget())) {
      if (ICF->isExitInst(Edge.getTarget())) {
        processExit(Edge);
      }
      if (!ICF->getSuccsOf(Edge.getTarget()).empty()) {
        processNormalFlow(Edge);
      }
    } else {
      processCall(Edge);
    }
  }

  // should be made a callable at some point
  void valuePropagationTask(const std::pair<n_t, d_t> NAndD) {
    n_t n = NAndD.first;
    // our initial seeds are not necessarily method-start points but here they
    // should be treated as such the same also for unbalanced return sites in
    // an unbalanced problem
    if (ICF->isStartPoint(n) || Seeds.containsInitialSeedsFor(n) ||
        UnbalancedRetSites.count(n)) {
      // FIXME: is currently not executed for main!!!
      // initial seeds are set in the global constructor, and main is also not
      // officially called by any other function
      propagateValueAtStart(NAndD, n);
    }
    if (ICF->isCallSite(n)) {
      propagateValueAtCall(NAndD, n);
    }
  }

  // should be made a callable at some point
  void valueComputationTask(const std::vector<n_t> &Values) {
    PAMM_GET_INSTANCE;
    for (n_t n : Values) {
      for (n_t SP : ICF->getStartPointsOf(ICF->getFunctionOf(n))) {
        using TableCell = typename Table<d_t, d_t, EdgeFunctionPtrType>::Cell;
        Table<d_t, d_t, EdgeFunctionPtrType> &LookupByTarget =
            JumpFn->lookupByTarget(n);
        for (const TableCell &SourceValTargetValAndFunction :
             LookupByTarget.cellSet()) {
          d_t dPrime = SourceValTargetValAndFunction.getRowKey();
          d_t d = SourceValTargetValAndFunction.getColumnKey();
          EdgeFunctionPtrType fPrime = SourceValTargetValAndFunction.getValue();
          l_t TargetVal = val(SP, dPrime);
          setVal(n, d,
                 IDEProblem.join(val(n, d),
                                 fPrime->computeTarget(std::move(TargetVal))));
          INC_COUNTER("Value Computation", 1, PAMM_SEVERITY_LEVEL::Full);
        }
      }
    }
  }

  virtual void saveEdges(n_t SourceNode, n_t SinkStmt, d_t SourceVal,
                         const container_type &DestVals, bool InterP) {
    if (!SolverConfig.recordEdges()) {
      return;
    }
    Table<n_t, n_t, std::map<d_t, container_type>> &TgtMap =
        (InterP) ? ComputedInterPathEdges : ComputedIntraPathEdges;
    TgtMap.get(SourceNode, SinkStmt)[SourceVal].insert(DestVals.begin(),
                                                       DestVals.end());
  }

  /// Computes the final values for edge functions.
  void computeValues() {
    PHASAR_LOG_LEVEL(DEBUG, "Start computing values");
    // Phase II(i)
    std::map<n_t, std::map<d_t, l_t>> AllSeeds = Seeds.getSeeds();
    for (n_t UnbalancedRetSite : UnbalancedRetSites) {
      if (AllSeeds.find(UnbalancedRetSite) == AllSeeds.end()) {
        AllSeeds[UnbalancedRetSite][ZeroValue] = IDEProblem.topElement();
      }
    }
    // do processing
    for (const auto &[StartPoint, Facts] : AllSeeds) {
      for (auto &[Fact, Value] : Facts) {
        PHASAR_LOG_LEVEL(DEBUG,
                         "set initial seed at: "
                             << IDEProblem.NtoString(StartPoint)
                             << ", fact: " << IDEProblem.DtoString(Fact)
                             << ", value: " << IDEProblem.LtoString(Value));
        // initialize the initial seeds with the top element as we have no
        // information at the beginning of the value computation problem
        setVal(StartPoint, Fact, Value);
        std::pair<n_t, d_t> SuperGraphNode(StartPoint, Fact);
        valuePropagationTask(SuperGraphNode);
      }
    }
    // Phase II(ii)
    // we create an array of all nodes and then dispatch fractions of this
    // array to multiple threads
    const std::set<n_t> AllNonCallStartNodes = ICF->allNonCallStartNodes();
    valueComputationTask(
        {AllNonCallStartNodes.begin(), AllNonCallStartNodes.end()});
  }

  /// Schedules the processing of initial seeds, initiating the analysis.
  /// Clients should only call this methods if performing synchronization on
  /// their own. Normally, solve() should be called instead.
  void submitInitialSeeds() {
    PAMM_GET_INSTANCE;
    // Check if the initial seeds contain the zero value at every starting
    // point. If not, the zero value needs to be added to allow for correct
    // solving of the problem.
    for (const auto &[StartPoint, Facts] : Seeds.getSeeds()) {
      if (Facts.find(ZeroValue) == Facts.end()) {
        // Add zero value if it's not in the set of facts.
        PHASAR_LOG_LEVEL(
            DEBUG, "Zero-Value has been added automatically to start point: "
                       << IDEProblem.NtoString(StartPoint));
        Seeds.addSeed(StartPoint, ZeroValue, IDEProblem.bottomElement());
      }
    }
    PHASAR_LOG_LEVEL(DEBUG,
                     "Number of initial seeds: " << Seeds.countInitialSeeds());
    PHASAR_LOG_LEVEL(DEBUG, "List of initial seeds: ");
    for (const auto &[StartPoint, Facts] : Seeds.getSeeds()) {
      PHASAR_LOG_LEVEL(DEBUG,
                       "Start point: " << IDEProblem.NtoString(StartPoint));
      /// If statically disabling the logger, Fact and Value are unused. To
      /// prevent the copilation to fail with -Werror, add the [[maybe_unused]]
      /// attribute
      for ([[maybe_unused]] const auto &[Fact, Value] : Facts) {
        PHASAR_LOG_LEVEL(DEBUG, "\tFact: " << IDEProblem.DtoString(Fact));
        PHASAR_LOG_LEVEL(DEBUG, "\tValue: " << IDEProblem.LtoString(Value));
      }
    }
    for (const auto &[StartPoint, Facts] : Seeds.getSeeds()) {
      for (const auto &[Fact, Value] : Facts) {
        PHASAR_LOG_LEVEL(
            DEBUG, "Submit seed at: " << IDEProblem.NtoString(StartPoint));
        PHASAR_LOG_LEVEL(DEBUG, "\tFact: " << IDEProblem.DtoString(Fact));
        PHASAR_LOG_LEVEL(DEBUG, "\tValue: " << IDEProblem.LtoString(Value));
        if (!IDEProblem.isZeroValue(Fact)) {
          INC_COUNTER("Gen facts", 1, PAMM_SEVERITY_LEVEL::Core);
        }
        propagate(Fact, StartPoint, Fact, EdgeIdentity<l_t>::getInstance(),
                  nullptr, false);
        JumpFn->addFunction(Fact, StartPoint, Fact,
                            EdgeIdentity<l_t>::getInstance());
      }
    }
  }

  /// Lines 21-32 of the algorithm.
  ///
  /// Stores callee-side summaries.
  /// Also, at the side of the caller, propagates intra-procedural flows to
  /// return sites using those newly computed summaries.
  ///
  /// @param edge an edge whose target node resembles a method exit
  ///
  virtual void processExit(const PathEdge<n_t, d_t> Edge) {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Process Exit", 1, PAMM_SEVERITY_LEVEL::Full);
    PHASAR_LOG_LEVEL(DEBUG, "Process exit at target: "
                                << IDEProblem.NtoString(Edge.getTarget()));
    n_t n = Edge.getTarget(); // an exit node; line 21...
    EdgeFunctionPtrType f = jumpFunction(Edge);
    f_t FunctionThatNeedsSummary = ICF->getFunctionOf(n);
    d_t d1 = Edge.factAtSource();
    d_t d2 = Edge.factAtTarget();
    // for each of the method's start points, determine incoming calls
    const std::set<n_t> StartPointsOf =
        ICF->getStartPointsOf(FunctionThatNeedsSummary);
    std::map<n_t, container_type> Inc;
    for (n_t SP : StartPointsOf) {
      // line 21.1 of Naeem/Lhotak/Rodriguez
      // register end-summary
      addEndSummary(SP, d1, n, d2, f);
      for (const auto &Entry : incoming(d1, SP)) {
        Inc[Entry.first] = Container{Entry.second};
      }
    }
    printEndSummaryTab();
    printIncomingTab();
    // for each incoming call edge already processed
    //(see processCall(..))
    for (const auto &Entry : Inc) {
      // line 22
      n_t c = Entry.first;
      // for each return site
      for (n_t RetSiteC : ICF->getReturnSitesOfCallAt(c)) {
        // compute return-flow function
        FlowFunctionPtrType RetFunction =
            CachedFlowEdgeFunctions.getRetFlowFunction(
                c, FunctionThatNeedsSummary, n, RetSiteC);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        // for each incoming-call value
        for (d_t d4 : Entry.second) {
          const container_type Targets =
              computeReturnFlowFunction(RetFunction, d1, d2, c, Entry.second);
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, RetSiteC, d2, Targets, true);
          // for each target value at the return site
          // line 23
          for (d_t d5 : Targets) {
            // compute composed function
            // get call edge function
            EdgeFunctionPtrType f4 =
                CachedFlowEdgeFunctions.getCallEdgeFunction(
                    c, d4, ICF->getFunctionOf(n), d1);
            PHASAR_LOG_LEVEL(DEBUG,
                             "Queried Call Edge Function: " << f4->str());
            // get return edge function
            EdgeFunctionPtrType f5 =
                CachedFlowEdgeFunctions.getReturnEdgeFunction(
                    c, ICF->getFunctionOf(n), n, d2, RetSiteC, d5);
            PHASAR_LOG_LEVEL(DEBUG,
                             "Queried Return Edge Function: " << f5->str());
            if (SolverConfig.emitESG()) {
              for (auto SP : ICF->getStartPointsOf(ICF->getFunctionOf(n))) {
                IntermediateEdgeFunctions[std::make_tuple(c, d4, SP, d1)]
                    .push_back(f4);
              }
              IntermediateEdgeFunctions[std::make_tuple(n, d2, RetSiteC, d5)]
                  .push_back(f5);
            }
            INC_COUNTER("EF Queries", 2, PAMM_SEVERITY_LEVEL::Full);
            // compose call function * function * return function
            PHASAR_LOG_LEVEL(DEBUG, "Compose: " << f5->str() << " * "
                                                << f->str() << " * "
                                                << f4->str());
            PHASAR_LOG_LEVEL(DEBUG, "         (return * function * call)");
            EdgeFunctionPtrType fPrime = f4->composeWith(f)->composeWith(f5);
            PHASAR_LOG_LEVEL(DEBUG, "       = " << fPrime->str());
            // for each jump function coming into the call, propagate to
            // return site using the composed function
            auto RevLookupResult = JumpFn->reverseLookup(c, d4);
            if (RevLookupResult) {
              for (size_t I = 0; I < RevLookupResult->get().size(); ++I) {
                auto ValAndFunc = RevLookupResult->get()[I];
                EdgeFunctionPtrType f3 = ValAndFunc.second;
                if (!f3->equal_to(AllTop)) {
                  d_t d3 = ValAndFunc.first;
                  d_t d5_restoredCtx = restoreContextOnReturnedFact(c, d4, d5);
                  PHASAR_LOG_LEVEL(DEBUG, "Compose: " << fPrime->str() << " * "
                                                      << f3->str());
                  propagate(d3, RetSiteC, d5_restoredCtx,
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
    if (SolverConfig.followReturnsPastSeeds() && Inc.empty() &&
        IDEProblem.isZeroValue(d1)) {
      const std::set<n_t> Callers = ICF->getCallersOf(FunctionThatNeedsSummary);
      for (n_t Caller : Callers) {
        for (n_t RetSiteC : ICF->getReturnSitesOfCallAt(Caller)) {
          FlowFunctionPtrType RetFunction =
              CachedFlowEdgeFunctions.getRetFlowFunction(
                  Caller, FunctionThatNeedsSummary, n, RetSiteC);
          INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
          const container_type Targets = computeReturnFlowFunction(
              RetFunction, d1, d2, Caller, Container{ZeroValue});
          ADD_TO_HISTOGRAM("Data-flow facts", targets.size(), 1,
                           PAMM_SEVERITY_LEVEL::Full);
          saveEdges(n, RetSiteC, d2, Targets, true);
          for (d_t d5 : Targets) {
            EdgeFunctionPtrType f5 =
                CachedFlowEdgeFunctions.getReturnEdgeFunction(
                    Caller, ICF->getFunctionOf(n), n, d2, RetSiteC, d5);
            PHASAR_LOG_LEVEL(DEBUG,
                             "Queried Return Edge Function: " << f5->str());
            if (SolverConfig.emitESG()) {
              IntermediateEdgeFunctions[std::make_tuple(n, d2, RetSiteC, d5)]
                  .push_back(f5);
            }
            INC_COUNTER("EF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
            PHASAR_LOG_LEVEL(DEBUG,
                             "Compose: " << f5->str() << " * " << f->str());
            propagteUnbalancedReturnFlow(RetSiteC, d5, f->composeWith(f5),
                                         Caller);
            // register for value processing (2nd IDE phase)
            UnbalancedRetSites.insert(RetSiteC);
          }
        }
      }
      // in cases where there are no callers, the return statement would
      // normally not be processed at all; this might be undesirable if
      // the flow function has a side effect such as registering a taint;
      // instead we thus call the return flow function will a null caller
      if (Callers.empty()) {
        FlowFunctionPtrType RetFunction =
            CachedFlowEdgeFunctions.getRetFlowFunction(
                nullptr, FunctionThatNeedsSummary, n, nullptr);
        INC_COUNTER("FF Queries", 1, PAMM_SEVERITY_LEVEL::Full);
        RetFunction->computeTargets(d2);
      }
    }
  }

  void propagteUnbalancedReturnFlow(n_t RetSiteC, d_t TargetVal,
                                    EdgeFunctionPtrType EdgeFunc,
                                    n_t RelatedCallSite) {
    propagate(ZeroValue, RetSiteC, TargetVal, std::move(EdgeFunc),
              RelatedCallSite, true);
  }

  /// This method will be called for each incoming edge and can be used to
  /// transfer knowledge from the calling edge to the returning edge, without
  /// affecting the summary edges at the callee.
  /// @param callSite
  ///
  /// @param d4
  ///            Fact stored with the incoming edge, i.e., present at the
  ///            caller side
  /// @param d5
  ///            Fact that originally should be propagated to the caller.
  /// @return Fact that will be propagated to the caller.
  ///
  d_t restoreContextOnReturnedFact(n_t /*CallSite*/, d_t /*d4*/, d_t d5) {
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

  /// Computes the normal flow function for the given set of start and end
  /// abstractions-
  /// @param flowFunction The normal flow function to compute
  /// @param d1 The abstraction at the method's start node
  /// @param d2 The abstraction at the current node
  /// @return The set of abstractions at the successor node
  ///
  container_type computeNormalFlowFunction(const FlowFunctionPtrType &FlowFunc,
                                           d_t /*d1*/, d_t d2) {
    return FlowFunc->computeTargets(d2);
  }

  container_type
  computeSummaryFlowFunction(const FlowFunctionPtrType &SummaryFlowFunction,
                             d_t /*d1*/, d_t d2) {
    return SummaryFlowFunction->computeTargets(d2);
  }

  /// Computes the call flow function for the given call-site abstraction
  /// @param callFlowFunction The call flow function to compute
  /// @param d1 The abstraction at the current method's start node.
  /// @param d2 The abstraction at the call site
  /// @return The set of caller-side abstractions at the callee's start node
  ///
  container_type
  computeCallFlowFunction(const FlowFunctionPtrType &CallFlowFunction,
                          d_t /*d1*/, d_t d2) {
    return CallFlowFunction->computeTargets(d2);
  }

  /// Computes the call-to-return flow function for the given call-site
  /// abstraction
  /// @param callToReturnFlowFunction The call-to-return flow function to
  /// compute
  /// @param d1 The abstraction at the current method's start node.
  /// @param d2 The abstraction at the call site
  /// @return The set of caller-side abstractions at the return site
  ///
  container_type computeCallToReturnFlowFunction(
      const FlowFunctionPtrType &CallToReturnFlowFunction, d_t /*d1*/, d_t d2) {
    return CallToReturnFlowFunction->computeTargets(d2);
  }

  /// Computes the return flow function for the given set of caller-side
  /// abstractions.
  /// @param retFunction The return flow function to compute
  /// @param d1 The abstraction at the beginning of the callee
  /// @param d2 The abstraction at the exit node in the callee
  /// @param callSite The call site
  /// @param callerSideDs The abstractions at the call site
  /// @return The set of caller-side abstractions at the return site
  ///
  container_type
  computeReturnFlowFunction(const FlowFunctionPtrType &RetFlowFunction,
                            d_t /*d1*/, d_t d2, n_t /*CallSite*/,
                            const Container & /*CallerSideDs*/) {
    return RetFlowFunction->computeTargets(d2);
  }

  /// Propagates the flow further down the exploded super graph, merging any
  /// edge function that might already have been computed for TargetVal at
  /// Target.
  ///
  /// @param SourceVal the source value of the propagated summary edge
  /// @param Target the target statement
  /// @param TargetVal the target value at the target statement
  /// @param f the new edge function computed from (s0,SourceVal) to
  /// (Target,TargetVal)
  /// @param relatedCallSite for call and return flows the related call
  /// statement, nullptr otherwise (this value is not used within this
  /// implementation but may be useful for subclasses of IDESolver)
  /// @param isUnbalancedReturn true if this edge is propagating an
  /// unbalanced return (this value is not used within this implementation
  /// but may be useful for subclasses of {@link IDESolver})
  ///
  void propagate(d_t SourceVal, n_t Target, d_t TargetVal,
                 const EdgeFunctionPtrType &f,
                 /* deliberately exposed to clients */
                 n_t /*RelatedCallSite*/,
                 /* deliberately exposed to clients */
                 bool /*IsUnbalancedReturn*/) {
    PHASAR_LOG_LEVEL(DEBUG, "Propagate flow");
    PHASAR_LOG_LEVEL(DEBUG,
                     "Source value  : " << IDEProblem.DtoString(SourceVal));
    PHASAR_LOG_LEVEL(DEBUG, "Target        : " << IDEProblem.NtoString(Target));
    PHASAR_LOG_LEVEL(DEBUG,
                     "Target value  : " << IDEProblem.DtoString(TargetVal));
    PHASAR_LOG_LEVEL(DEBUG,
                     "Edge function : " << f.get()->str()
                                        << " (result of previous compose)");

    EdgeFunctionPtrType JumpFnE = [&]() {
      const auto RevLookupResult = JumpFn->reverseLookup(Target, TargetVal);
      if (RevLookupResult) {
        const auto &JumpFnContainer = RevLookupResult->get();
        const auto Find = std::find_if(
            JumpFnContainer.begin(), JumpFnContainer.end(),
            [SourceVal](auto &KVpair) { return KVpair.first == SourceVal; });
        if (Find != JumpFnContainer.end()) {
          return Find->second;
        }
      }
      // jump function is initialized to all-top if no entry
      // was found
      return AllTop;
    }();
    EdgeFunctionPtrType fPrime = JumpFnE->joinWith(f);
    bool NewFunction = !(fPrime->equal_to(JumpFnE));

    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(
            DEBUG,
            "Join: " << JumpFnE->str() << " & " << f.get()->str()
                     << (JumpFnE->equal_to(f) ? " (EF's are equal)" : " "));
        PHASAR_LOG_LEVEL(DEBUG,
                         "    = " << fPrime->str()
                                  << (NewFunction ? " (new jump func)" : " "));
        PHASAR_LOG_LEVEL(DEBUG, ' '));
    if (NewFunction) {
      JumpFn->addFunction(SourceVal, Target, TargetVal, fPrime);
      const PathEdge<n_t, d_t> Edge(SourceVal, Target, TargetVal);
      PathEdgeCount++;
      pathEdgeProcessingTask(Edge);

      IF_LOG_ENABLED(if (!IDEProblem.isZeroValue(TargetVal)) {
        PHASAR_LOG_LEVEL(
            DEBUG, "EDGE: <F: " << Target->getFunction()->getName() << ", D: "
                                << IDEProblem.DtoString(SourceVal) << '>');
        PHASAR_LOG_LEVEL(DEBUG,
                         " ---> <N: " << IDEProblem.NtoString(Target) << ',');
        PHASAR_LOG_LEVEL(DEBUG, "       D: " << IDEProblem.DtoString(TargetVal)
                                             << ',');
        PHASAR_LOG_LEVEL(DEBUG, "      EF: " << fPrime->str() << '>');
        PHASAR_LOG_LEVEL(DEBUG, ' ');
      });
    } else {
      PHASAR_LOG_LEVEL(DEBUG, "PROPAGATE: No new function!");
    }
  }

  l_t joinValueAt(n_t /*Unit*/, d_t /*Fact*/, l_t Curr, l_t NewVal) {
    return IDEProblem.join(std::move(Curr), std::move(NewVal));
  }

  std::set<typename Table<n_t, d_t, EdgeFunctionPtrType>::Cell>
  endSummary(n_t SP, d_t d3) {
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Core) {
      auto Key = std::make_pair(SP, d3);
      auto FindND = FSummaryReuse.find(Key);
      if (FindND == FSummaryReuse.end()) {
        FSummaryReuse.emplace(Key, 0);
      } else {
        FSummaryReuse[Key] += 1;
      }
    }
    return EndsummaryTab.get(SP, d3).cellSet();
  }

  std::map<n_t, container_type> incoming(d_t d1, n_t SP) {
    return IncomingTab.get(SP, d1);
  }

  void addIncoming(n_t SP, d_t d3, n_t n, d_t d2) {
    IncomingTab.get(SP, d3)[n].insert(d2);
  }

  void printIncomingTab() const {
#ifdef DYNAMIC_LOG
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Start of incomingtab entry");
        for (const auto &Cell
             : IncomingTab.cellSet()) {
          PHASAR_LOG_LEVEL(DEBUG,
                           "sP: " << IDEProblem.NtoString(Cell.getRowKey()));
          PHASAR_LOG_LEVEL(DEBUG,
                           "d3: " << IDEProblem.DtoString(Cell.getColumnKey()));
          for (const auto &Entry : Cell.getValue()) {
            PHASAR_LOG_LEVEL(DEBUG,
                             "  n: " << IDEProblem.NtoString(Entry.first));
            for (const auto &Fact : Entry.second) {
              PHASAR_LOG_LEVEL(DEBUG, "  d2: " << IDEProblem.DtoString(Fact));
            }
          }
          PHASAR_LOG_LEVEL(DEBUG, "---------------");
        } PHASAR_LOG_LEVEL(DEBUG, "End of incomingtab entry");)
#endif
  }

  void printEndSummaryTab() const {
#ifdef DYNAMIC_LOG
    IF_LOG_ENABLED(
        PHASAR_LOG_LEVEL(DEBUG, "Start of endsummarytab entry");
        for (const auto &Cell
             : EndsummaryTab.cellVec()) {
          PHASAR_LOG_LEVEL(DEBUG,
                           "sP: " << IDEProblem.NtoString(Cell.getRowKey()));
          PHASAR_LOG_LEVEL(DEBUG,
                           "d1: " << IDEProblem.DtoString(Cell.getColumnKey()));
          for (const auto &InnerCell : Cell.getValue().cellVec()) {
            PHASAR_LOG_LEVEL(
                DEBUG, "  eP: " << IDEProblem.NtoString(InnerCell.getRowKey()));
            PHASAR_LOG_LEVEL(DEBUG, "  d2: " << IDEProblem.DtoString(
                                        InnerCell.getColumnKey()));
            PHASAR_LOG_LEVEL(DEBUG, "  EF: " << InnerCell.getValue()->str());
          }
          PHASAR_LOG_LEVEL(DEBUG, "---------------");
        } PHASAR_LOG_LEVEL(DEBUG, "End of endsummarytab entry");)
#endif
  }

  void printComputedPathEdges() {
    llvm::outs()
        << "\n**********************************************************";
    llvm::outs()
        << "\n*          Computed intra-procedural path egdes          *";
    llvm::outs()
        << "\n**********************************************************\n";

    // Sort intra-procedural path edges
    auto Cells = ComputedIntraPathEdges.cellVec();
    StmtLess Stmtless(ICF);
    sort(Cells.begin(), Cells.end(), [&Stmtless](auto Lhs, auto Rhs) {
      return Stmtless(Lhs.getRowKey(), Rhs.getRowKey());
    });
    for (const auto &Cell : Cells) {
      auto Edge = std::make_pair(Cell.getRowKey(), Cell.getColumnKey());
      std::string N2Label = IDEProblem.NtoString(Edge.second);
      llvm::outs() << "\nN1: " << IDEProblem.NtoString(Edge.first) << '\n'
                   << "N2: " << N2Label << "\n----"
                   << std::string(N2Label.size(), '-') << '\n';
      for (auto D1ToD2Set : Cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        llvm::outs() << "D1: " << IDEProblem.DtoString(D1Fact) << '\n';
        for (auto D2Fact : D1ToD2Set.second) {
          llvm::outs() << "\tD2: " << IDEProblem.DtoString(D2Fact) << '\n';
        }
        llvm::outs() << '\n';
      }
    }

    llvm::outs()
        << "\n**********************************************************";
    llvm::outs()
        << "\n*          Computed inter-procedural path edges          *";
    llvm::outs()
        << "\n**********************************************************\n";

    // Sort intra-procedural path edges
    Cells = ComputedInterPathEdges.cellVec();
    sort(Cells.begin(), Cells.end(), [&Stmtless](auto Lhs, auto Rhs) {
      return Stmtless(Lhs.getRowKey(), Rhs.getRowKey());
    });
    for (const auto &Cell : Cells) {
      auto Edge = std::make_pair(Cell.getRowKey(), Cell.getColumnKey());
      std::string N2Label = IDEProblem.NtoString(Edge.second);
      llvm::outs() << "\nN1: " << IDEProblem.NtoString(Edge.first) << '\n'
                   << "N2: " << N2Label << "\n----"
                   << std::string(N2Label.size(), '-') << '\n';
      for (auto D1ToD2Set : Cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        llvm::outs() << "D1: " << IDEProblem.DtoString(D1Fact) << '\n';
        for (auto D2Fact : D1ToD2Set.second) {
          llvm::outs() << "\tD2: " << IDEProblem.DtoString(D2Fact) << '\n';
        }
        llvm::outs() << '\n';
      }
    }
  }

  /// The invariant for computing the number of generated (#gen) and killed
  /// (#kill) facts:
  ///   (1) #Valid facts at the last statement <= #gen - #kill
  ///   (2) #gen >= #kill
  ///
  /// The total number of valid facts can be smaller than the difference of
  /// generated and killed facts, due to set semantics, i.e., a fact can be
  /// generated multiple times but appears only once.
  ///
  /// Zero value is not counted!
  ///
  /// @brief Computes and prints statistics of the analysis run, e.g. number of
  /// generated/killed facts, number of summary-reuses etc.
  ///
  void computeAndPrintStatistics() {
    PAMM_GET_INSTANCE;
    // Stores all valid facts at return site in caller context; return-site is
    // key
    std::unordered_map<n_t, std::set<d_t>> ValidInCallerContext;
    size_t NumGenFacts = 0;
    size_t NumKillFacts = 0;
    size_t NumIntraPathEdges = 0;
    size_t NumInterPathEdges = 0;
    // --- Intra-procedural Path Edges ---
    // d1 --> d2-Set
    // Case 1: d1 in d2-Set
    // Case 2: d1 not in d2-Set, i.e., d1 was killed. d2-Set could be empty.
    for (const auto &Cell : ComputedIntraPathEdges.cellSet()) {
      auto Edge = std::make_pair(Cell.getRowKey(), Cell.getColumnKey());
      PHASAR_LOG_LEVEL(DEBUG, "N1: " << IDEProblem.NtoString(Edge.first));
      PHASAR_LOG_LEVEL(DEBUG, "N2: " << IDEProblem.NtoString(Edge.second));
      for (auto &[D1, D2s] : Cell.getValue()) {
        PHASAR_LOG_LEVEL(DEBUG, "d1: " << IDEProblem.DtoString(D1));
        NumIntraPathEdges += D2s.size();
        // Case 1
        if (D2s.find(D1) != D2s.end()) {
          NumGenFacts += D2s.size() - 1;
        }
        // Case 2
        else {
          NumGenFacts += D2s.size();
          // We ignore the zero value
          if (!IDEProblem.isZeroValue(D1)) {
            NumKillFacts++;
          }
        }
        // Store all valid facts after call-to-return flow
        if (ICF->isCallSite(Edge.first)) {
          ValidInCallerContext[Edge.second].insert(D2s.begin(), D2s.end());
        }
        IF_LOG_ENABLED([this](const auto &D2s) {
          for (auto D2 : D2s) {
            PHASAR_LOG_LEVEL(DEBUG, "d2: " << IDEProblem.DtoString(D2));
          }
          PHASAR_LOG_LEVEL(DEBUG, "----");
        }(D2s));
      }
      PHASAR_LOG_LEVEL(DEBUG, " ");
    }
    // Stores all pairs of (Startpoint, Fact) for which a summary was applied
    std::set<std::pair<n_t, d_t>> ProcessSummaryFacts;
    PHASAR_LOG_LEVEL(DEBUG, "==============================================");
    PHASAR_LOG_LEVEL(DEBUG, "INTER PATH EDGES");
    for (const auto &Cell : ComputedInterPathEdges.cellSet()) {
      auto Edge = std::make_pair(Cell.getRowKey(), Cell.getColumnKey());
      PHASAR_LOG_LEVEL(DEBUG, "N1: " << IDEProblem.NtoString(Edge.first));
      PHASAR_LOG_LEVEL(DEBUG, "N2: " << IDEProblem.NtoString(Edge.second));
      // --- Call-flow Path Edges ---
      // Case 1: d1 --> empty set
      //   Can be ignored, since killing a fact in the caller context will
      //   actually happen during  call-to-return.
      //
      // Case 2: d1 --> d2-Set
      //   Every fact d_i != ZeroValue in d2-set will be generated in the
      // callee context, thus counts as a new fact. Even if d1 is passed as it
      // is, it will count as a new fact. The reason for this is, that d1 can
      // be killed in the callee context, but still be valid in the caller
      // context.
      //
      // Special Case: Summary was applied for a particular call
      //   Process the summary's #gen and #kill.
      if (ICF->isCallSite(Edge.first)) {
        for (auto &[D1, D2s] : Cell.getValue()) {
          PHASAR_LOG_LEVEL(DEBUG, "d1: " << IDEProblem.DtoString(D1));
          NumInterPathEdges += D2s.size();
          for (auto D2 : D2s) {
            if (!IDEProblem.isZeroValue(D2)) {
              NumGenFacts++;
            }
            // Special case
            if (ProcessSummaryFacts.find(std::make_pair(Edge.second, D2)) !=
                ProcessSummaryFacts.end()) {
              std::multiset<d_t> SummaryDMultiSet =
                  EndsummaryTab.get(Edge.second, D2).columnKeySet();
              // remove duplicates from multiset
              std::set<d_t> SummaryDSet(SummaryDMultiSet.begin(),
                                        SummaryDMultiSet.end());
              // Process summary just as an intra-procedural edge
              if (SummaryDSet.find(D2) != SummaryDSet.end()) {
                NumGenFacts += SummaryDSet.size() - 1;
              } else {
                NumGenFacts += SummaryDSet.size();
                // We ignore the zero value
                if (!IDEProblem.isZeroValue(D1)) {
                  NumKillFacts++;
                }
              }
            } else {
              ProcessSummaryFacts.emplace(Edge.second, D2);
            }
            PHASAR_LOG_LEVEL(DEBUG, "d2: " << IDEProblem.DtoString(D2));
          }
          PHASAR_LOG_LEVEL(DEBUG, "----");
        }
      }
      // --- Return-flow Path Edges ---
      // Since every fact passed to the callee was counted as a new fact, we
      // have to count every fact propagated to the caller as a kill to
      // satisfy our invariant. Obviously, every fact not propagated to the
      // caller will count as a kill. If an actual new fact is propagated to
      // the caller, we have to increase the number of generated facts by one.
      // Zero value does not count towards generated/killed facts.
      if (ICF->isExitInst(Cell.getRowKey())) {
        for (auto &[D1, D2s] : Cell.getValue()) {
          PHASAR_LOG_LEVEL(DEBUG, "d1: " << IDEProblem.DtoString(D1));
          NumInterPathEdges += D2s.size();
          auto CallerFacts = ValidInCallerContext[Edge.second];
          for (auto D2 : D2s) {
            // d2 not valid in caller context
            if (CallerFacts.find(D2) == CallerFacts.end()) {
              NumGenFacts++;
            }
            PHASAR_LOG_LEVEL(DEBUG, "d2: " << IDEProblem.DtoString(D2));
          }
          if (!IDEProblem.isZeroValue(D1)) {
            NumKillFacts++;
          }
          PHASAR_LOG_LEVEL(DEBUG, "----");
        }
      }
      PHASAR_LOG_LEVEL(DEBUG, " ");
    }
    PHASAR_LOG_LEVEL(DEBUG, "SUMMARY REUSE");
    std::size_t TotalSummaryReuse = 0;
    for (const auto &Entry : FSummaryReuse) {
      PHASAR_LOG_LEVEL(DEBUG,
                       "N1: " << IDEProblem.NtoString(Entry.first.first));
      PHASAR_LOG_LEVEL(DEBUG,
                       "D1: " << IDEProblem.DtoString(Entry.first.second));
      PHASAR_LOG_LEVEL(DEBUG, "#Reuse: " << Entry.second);
      TotalSummaryReuse += Entry.second;
    }
    INC_COUNTER("Gen facts", NumGenFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Kill facts", NumKillFacts, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Summary-reuse", TotalSummaryReuse, PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Intra Path Edges", NumIntraPathEdges,
                PAMM_SEVERITY_LEVEL::Core);
    INC_COUNTER("Inter Path Edges", NumInterPathEdges,
                PAMM_SEVERITY_LEVEL::Core);

    PHASAR_LOG_LEVEL(INFO, "----------------------------------------------");
    PHASAR_LOG_LEVEL(INFO, "=== Solver Statistics ===");
    PHASAR_LOG_LEVEL(INFO, "#Facts generated : " << GET_COUNTER("Gen facts"));
    PHASAR_LOG_LEVEL(INFO, "#Facts killed    : " << GET_COUNTER("Kill facts"));
    PHASAR_LOG_LEVEL(INFO,
                     "#Summary-reuse   : " << GET_COUNTER("Summary-reuse"));
    PHASAR_LOG_LEVEL(INFO,
                     "#Intra Path Edges: " << GET_COUNTER("Intra Path Edges"));
    PHASAR_LOG_LEVEL(INFO,
                     "#Inter Path Edges: " << GET_COUNTER("Inter Path Edges"));
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      PHASAR_LOG_LEVEL(
          INFO, "Flow function query count: " << GET_COUNTER("FF Queries"));
      PHASAR_LOG_LEVEL(
          INFO, "Edge function query count: " << GET_COUNTER("EF Queries"));
      PHASAR_LOG_LEVEL(INFO, "Data-flow value propagation count: "
                                 << GET_COUNTER("Value Propagation"));
      PHASAR_LOG_LEVEL(INFO, "Data-flow value computation count: "
                                 << GET_COUNTER("Value Computation"));
      PHASAR_LOG_LEVEL(INFO,
                       "Special flow function usage count: "
                           << GET_COUNTER("SpecialSummary-FF Application"));
      PHASAR_LOG_LEVEL(INFO, "Jump function construciton count: "
                                 << GET_COUNTER("JumpFn Construction"));
      PHASAR_LOG_LEVEL(INFO,
                       "Phase I duration: " << PRINT_TIMER("DFA Phase I"));
      PHASAR_LOG_LEVEL(INFO,
                       "Phase II duration: " << PRINT_TIMER("DFA Phase II"));
      PHASAR_LOG_LEVEL(INFO, "----------------------------------------------");
      CachedFlowEdgeFunctions.print();
    }
  }

public:
  void enableESGAsDot() { SolverConfig.setEmitESG(); }

  void
  emitESGAsDot(llvm::raw_ostream &OS = llvm::outs(),
               std::string DotConfigDir = PhasarConfig::PhasarDirectory()) {
    PHASAR_LOG_LEVEL(DEBUG, "Emit Exploded super-graph (ESG) as DOT graph");
    PHASAR_LOG_LEVEL(DEBUG, "Process intra-procedural path egdes");
    PHASAR_LOG_LEVEL(DEBUG, "=============================================");
    DOTGraph<d_t> G;
    DOTConfig::importDOTConfig(std::move(DotConfigDir));
    DOTFunctionSubGraph *FG = nullptr;

    // Sort intra-procedural path edges
    auto Cells = ComputedIntraPathEdges.cellVec();
    StmtLess Stmtless(ICF);
    sort(Cells.begin(), Cells.end(), [&Stmtless](auto Lhs, auto Rhs) {
      return Stmtless(Lhs.getRowKey(), Rhs.getRowKey());
    });
    for (const auto &Cell : Cells) {
      auto Edge = std::make_pair(Cell.getRowKey(), Cell.getColumnKey());
      std::string N1Label = IDEProblem.NtoString(Edge.first);
      std::string N2Label = IDEProblem.NtoString(Edge.second);
      PHASAR_LOG_LEVEL(DEBUG, "N1: " << N1Label);
      PHASAR_LOG_LEVEL(DEBUG, "N2: " << N2Label);
      std::string N1StmtId = ICF->getStatementId(Edge.first);
      std::string N2StmtId = ICF->getStatementId(Edge.second);
      std::string FuncName = ICF->getFunctionOf(Edge.first)->getName().str();
      // Get or create function subgraph
      if (!FG || FG->Id != FuncName) {
        FG = &G.Functions[FuncName];
        FG->Id = FuncName;
      }

      // Create control flow nodes
      DOTNode N1(FuncName, N1Label, N1StmtId);
      DOTNode N2(FuncName, N2Label, N2StmtId);
      // Add control flow node(s) to function subgraph
      FG->Stmts.insert(N1);
      if (ICF->isExitInst(Edge.second)) {
        FG->Stmts.insert(N2);
      }

      // Set control flow edge
      FG->IntraCFEdges.emplace(N1, N2);

      DOTFactSubGraph *D1FSG = nullptr;
      unsigned D1FactId = 0;
      unsigned D2FactId = 0;
      for (const auto &D1ToD2Set : Cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        PHASAR_LOG_LEVEL(DEBUG, "d1: " << IDEProblem.DtoString(D1Fact));

        DOTNode D1;
        if (IDEProblem.isZeroValue(D1Fact)) {
          D1 = {FuncName, "Λ", N1StmtId, 0, false, true};
          D1FactId = 0;
        } else {
          // Get the fact-ID
          D1FactId = G.getFactID(D1Fact);
          std::string D1Label = IDEProblem.DtoString(D1Fact);

          // Get or create the fact subgraph
          D1FSG = FG->getOrCreateFactSG(D1FactId, D1Label);

          // Insert D1 to fact subgraph
          D1 = {FuncName, D1Label, N1StmtId, D1FactId, false, true};
          D1FSG->Nodes.insert(std::make_pair(N1StmtId, D1));
        }

        DOTFactSubGraph *D2FSG = nullptr;
        for (const auto &D2Fact : D1ToD2Set.second) {
          PHASAR_LOG_LEVEL(DEBUG, "d2: " << IDEProblem.DtoString(D2Fact));
          // We do not need to generate any intra-procedural nodes and edges
          // for the zero value since they will be auto-generated
          if (!IDEProblem.isZeroValue(D2Fact)) {
            // Get the fact-ID
            D2FactId = G.getFactID(D2Fact);
            std::string D2Label = IDEProblem.DtoString(D2Fact);
            DOTNode D2 = {FuncName, D2Label, N2StmtId, D2FactId, false, true};
            std::string EFLabel;
            auto EFVec = IntermediateEdgeFunctions[std::make_tuple(
                Edge.first, D1Fact, Edge.second, D2Fact)];
            for (const auto &EF : EFVec) {
              EFLabel += EF->str() + ", ";
            }
            PHASAR_LOG_LEVEL(DEBUG, "EF LABEL: " << EFLabel);
            if (D1FactId == D2FactId && !IDEProblem.isZeroValue(D1Fact)) {
              assert(D1FSG && "D1_FSG was nullptr but should be valid.");
              D1FSG->Nodes.insert(std::make_pair(N2StmtId, D2));
              D1FSG->Edges.emplace(D1, D2, true, EFLabel);
            } else {
              // Get or create the fact subgraph
              D2FSG = FG->getOrCreateFactSG(D2FactId, D2Label);

              D2FSG->Nodes.insert(std::make_pair(N2StmtId, D2));
              FG->CrossFactEdges.emplace(D1, D2, true, EFLabel);
            }
          }
        }
        PHASAR_LOG_LEVEL(DEBUG, "----------");
      }
      PHASAR_LOG_LEVEL(DEBUG, " ");
    }

    PHASAR_LOG_LEVEL(DEBUG, "=============================================");
    PHASAR_LOG_LEVEL(DEBUG, "Process inter-procedural path edges");
    PHASAR_LOG_LEVEL(DEBUG, "=============================================");
    Cells = ComputedInterPathEdges.cellVec();
    sort(Cells.begin(), Cells.end(), [&Stmtless](auto Lhs, auto Rhs) {
      return Stmtless(Lhs.getRowKey(), Rhs.getRowKey());
    });
    for (const auto &Cell : Cells) {
      auto Edge = std::make_pair(Cell.getRowKey(), Cell.getColumnKey());
      std::string N1Label = IDEProblem.NtoString(Edge.first);
      std::string N2Label = IDEProblem.NtoString(Edge.second);
      std::string FNameOfN1 = ICF->getFunctionOf(Edge.first)->getName().str();
      std::string FNameOfN2 = ICF->getFunctionOf(Edge.second)->getName().str();
      std::string N1StmtId = ICF->getStatementId(Edge.first);
      std::string N2StmtId = ICF->getStatementId(Edge.second);
      PHASAR_LOG_LEVEL(DEBUG, "N1: " << N1Label);
      PHASAR_LOG_LEVEL(DEBUG, "N2: " << N2Label);

      // Add inter-procedural control flow edge
      DOTNode N1(FNameOfN1, N1Label, N1StmtId);
      DOTNode N2(FNameOfN2, N2Label, N2StmtId);

      // Handle recursion control flow as intra-procedural control flow
      // since those eges never leave the function subgraph
      FG = nullptr;
      if (FNameOfN1 == FNameOfN2) {
        // This function subgraph is guaranteed to exist
        FG = &G.Functions[FNameOfN1];
        FG->IntraCFEdges.emplace(N1, N2);
      } else {
        // Check the case where the callee is a single statement function,
        // thus does not contain intra-procedural path edges. We have to
        // generate the function sub graph here!
        if (!G.Functions.count(FNameOfN1)) {
          FG = &G.Functions[FNameOfN1];
          FG->Id = FNameOfN1;
          FG->Stmts.insert(N1);
        } else if (!G.Functions.count(FNameOfN2)) {
          FG = &G.Functions[FNameOfN2];
          FG->Id = FNameOfN2;
          FG->Stmts.insert(N2);
        }
        G.InterCFEdges.emplace(N1, N2);
      }

      // Create D1 and D2, if D1 == D2 == lambda then add Edge(D1, D2) to
      // interLambdaEges otherwise add Edge(D1, D2) to interFactEdges
      unsigned D1FactId = 0;
      unsigned D2FactId = 0;
      for (const auto &D1ToD2Set : Cell.getValue()) {
        auto D1Fact = D1ToD2Set.first;
        PHASAR_LOG_LEVEL(DEBUG, "d1: " << IDEProblem.DtoString(D1Fact));
        DOTNode D1;
        if (IDEProblem.isZeroValue(D1Fact)) {
          D1 = {FNameOfN1, "Λ", N1StmtId, 0, false, true};
        } else {
          // Get the fact-ID
          D1FactId = G.getFactID(D1Fact);
          std::string D1Label = IDEProblem.DtoString(D1Fact);
          D1 = {FNameOfN1, D1Label, N1StmtId, D1FactId, false, true};
          // FG should already exist even for single statement functions
          if (!G.containsFactSG(FNameOfN1, D1FactId)) {
            FG = &G.Functions[FNameOfN1];
            auto *D1FSG = FG->getOrCreateFactSG(D1FactId, D1Label);
            D1FSG->Nodes.insert(std::make_pair(N1StmtId, D1));
          }
        }

        auto D2Set = D1ToD2Set.second;
        for (const auto &D2Fact : D2Set) {
          PHASAR_LOG_LEVEL(DEBUG, "d2: " << IDEProblem.DtoString(D2Fact));
          DOTNode D2;
          if (IDEProblem.isZeroValue(D2Fact)) {
            D2 = {FNameOfN2, "Λ", N2StmtId, 0, false, true};
          } else {
            // Get the fact-ID
            D2FactId = G.getFactID(D2Fact);
            std::string D2Label = IDEProblem.DtoString(D2Fact);
            D2 = {FNameOfN2, D2Label, N2StmtId, D2FactId, false, true};
            // FG should already exist even for single statement functions
            if (!G.containsFactSG(FNameOfN2, D2FactId)) {
              FG = &G.Functions[FNameOfN2];
              auto *D2FSG = FG->getOrCreateFactSG(D2FactId, D2Label);
              D2FSG->Nodes.insert(std::make_pair(N2StmtId, D2));
            }
          }

          if (IDEProblem.isZeroValue(D1Fact) &&
              IDEProblem.isZeroValue(D2Fact)) {
            // Do not add lambda recursion edges as inter-procedural edges
            if (D1.FuncName != D2.FuncName) {
              G.InterLambdaEdges.emplace(D1, D2, true, "AllBottom", "BOT");
            }
          } else {
            // std::string EFLabel = EF ? EF->str() : " ";
            std::string EFLabel;
            auto EFVec = IntermediateEdgeFunctions[std::make_tuple(
                Edge.first, D1Fact, Edge.second, D2Fact)];
            for (const auto &EF : EFVec) {
              PHASAR_LOG_LEVEL(DEBUG, "Partial EF Label: " << EF->str());
              EFLabel.append(EF->str() + ", ");
            }
            PHASAR_LOG_LEVEL(DEBUG, "EF LABEL: " << EFLabel);
            G.InterFactEdges.emplace(D1, D2, true, EFLabel);
          }
        }
        PHASAR_LOG_LEVEL(DEBUG, "----------");
      }
      PHASAR_LOG_LEVEL(DEBUG, " ");
    }
    OS << G;
  }

  /// @brief: Allows less-than comparison based on the statement ID.
  struct StmtLess {
    const i_t *ICF;
    StringIDLess StrIDLess;
    StmtLess(const i_t *ICF) : ICF(ICF), StrIDLess(StringIDLess()) {}
    bool operator()(n_t Lhs, n_t Rhs) {
      return StrIDLess(ICF->getStatementId(Lhs), ICF->getStatementId(Rhs));
    }
  };
};

template <typename AnalysisDomainTy, typename Container>
llvm::raw_ostream &
operator<<(llvm::raw_ostream &OS,
           const IDESolver<AnalysisDomainTy, Container> &Solver) {
  Solver.dumpResults(OS);
  return OS;
}

template <typename Problem>
IDESolver(Problem &) -> IDESolver<typename Problem::ProblemAnalysisDomain,
                                  typename Problem::container_type>;

template <typename Problem>
using IDESolver_P = IDESolver<typename Problem::ProblemAnalysisDomain,
                              typename Problem::container_type>;

} // namespace psr

#endif
