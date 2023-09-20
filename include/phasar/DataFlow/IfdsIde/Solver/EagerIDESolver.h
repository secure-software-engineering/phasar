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

#ifndef PHASAR_DATAFLOW_IFDSIDE_SOLVER_EAGERIDESOLVER_H
#define PHASAR_DATAFLOW_IFDSIDE_SOLVER_EAGERIDESOLVER_H

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDBBase.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolverAPIMixin.h"
#include "phasar/DataFlow/IfdsIde/Solver/JumpFunctions.h"
#include "phasar/DataFlow/IfdsIde/Solver/SolverStrategy.h"
#include "phasar/DataFlow/IfdsIde/Solver/detail/IDESolverImpl.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/Utils/DOTGraph.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Table.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace psr {

/// Solves the given IDETabulationProblem as described in the 1996 paper by
/// Sagiv, Horwitz and Reps. To solve the problem, call solve(). Results
/// can then be queried by using resultAt() and resultsAt().
///
/// Propagates data-flow facts onto the statement, where they were generated.
template <typename AnalysisDomainTy, typename Container>
class IDESolver<AnalysisDomainTy, Container, PropagateOntoStrategy>
    : public IDESolverImpl<
          IDESolver<AnalysisDomainTy, Container, PropagateOntoStrategy>,
          AnalysisDomainTy, Container, PropagateOntoStrategy> {
  using base_t = IDESolverImpl<
      IDESolver<AnalysisDomainTy, Container, PropagateOntoStrategy>,
      AnalysisDomainTy, Container, PropagateOntoStrategy>;

public:
  using ProblemTy = IDETabulationProblem<AnalysisDomainTy, Container>;
  using container_type = typename ProblemTy::container_type;
  using FlowFunctionPtrType = typename ProblemTy::FlowFunctionPtrType;

  using l_t = typename AnalysisDomainTy::l_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;

  explicit IDESolver(IDETabulationProblem<AnalysisDomainTy, Container> &Problem,
                     const i_t *ICF, PropagateOntoStrategy Strategy = {})
      : base_t(Problem, ICF, Strategy) {}

private:
  friend base_t;
  friend IDESolverAPIMixin<
      IDESolver<AnalysisDomainTy, Container, PropagateOntoStrategy>>;

  /// -- Phase I customization

  bool updateJumpFunction(d_t SourceVal, n_t Target, d_t TargetVal,
                          EdgeFunction<l_t> *f) {
    EdgeFunction<l_t> JumpFnE = [&]() {
      const auto RevLookupResult =
          this->JumpFn->reverseLookup(Target, TargetVal);
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
      return this->AllTop;
    }();

    EdgeFunction<l_t> fPrime = JumpFnE.joinWith(*f);
    bool NewFunction = fPrime != JumpFnE;
    if (NewFunction) {
      IF_LOG_ENABLED(
          PHASAR_LOG_LEVEL(
              DEBUG, "Join: " << JumpFnE << " & " << *f
                              << (JumpFnE == *f ? " (EF's are equal)" : ""));
          PHASAR_LOG_LEVEL(
              DEBUG, "    = " << f << (NewFunction ? " (new jump func)" : ""));
          PHASAR_LOG_LEVEL(DEBUG, ' '));

      *f = fPrime;
      this->JumpFn->addFunction(std::move(SourceVal), std::move(Target),
                                std::move(TargetVal), std::move(fPrime));

      IF_LOG_ENABLED(if (!this->IDEProblem.isZeroValue(TargetVal)) {
        PHASAR_LOG_LEVEL(
            DEBUG, "EDGE: <F: " << FToString(this->ICF->getFunctionOf(Target))
                                << ", D: " << DToString(SourceVal) << '>');
        PHASAR_LOG_LEVEL(DEBUG, " ---> <N: " << NToString(Target) << ',');
        PHASAR_LOG_LEVEL(DEBUG, "       D: " << DToString(TargetVal) << ',');
        PHASAR_LOG_LEVEL(DEBUG, "      EF: " << fPrime << '>');
        PHASAR_LOG_LEVEL(DEBUG, ' ');
      });
    }
    return NewFunction;
  }

  template <typename TargetsT>
  void updateWithNewEdges(d_t SourceVal, n_t OldTarget, n_t /*NewTarget*/,
                          const TargetsT &NewTargets, d_t TargetVal,
                          EdgeFunction<l_t> EF) {
    if (updateJumpFunction(SourceVal, OldTarget, TargetVal, &EF)) {
      auto It = NewTargets.begin();
      auto End = NewTargets.end();
      if (It == End) {
        return;
      }

      auto Next = std::next(It);
      if (Next == End) {
        addWorklistItem(SourceVal, *It, std::move(TargetVal), std::move(EF));
        return;
      }

      for (; It != End; ++It) {
        addWorklistItem(SourceVal, *It, TargetVal, EF);
      }
    }
  }

  void addInitialWorklistItem(d_t SourceVal, n_t Target, d_t TargetVal,
                              EdgeFunction<l_t> EF) {
    addWorklistItem(std::move(SourceVal), std::move(Target),
                    std::move(TargetVal), std::move(EF));
  }

  void addWorklistItem(d_t SourceVal, n_t Target, d_t TargetVal,
                       EdgeFunction<l_t> EF) {
    WorkList.emplace_back(
        PathEdge{std::move(SourceVal), std::move(Target), std::move(TargetVal)},
        std::move(EF));
  }

  bool doNext() {
    assert(!WorkList.empty());
    auto [Edge, EF] = std::move(WorkList.back());
    WorkList.pop_back();

    this->propagate(std::move(Edge), std::move(EF));

    return !WorkList.empty();
  }

  /// -- Phase II customization

  void propagateValueAtStart(const std::pair<n_t, d_t> NAndD, n_t Stmt) {
    PAMM_GET_INSTANCE;
    d_t Fact = NAndD.second;
    f_t Func = this->ICF->getFunctionOf(Stmt);
    for (const n_t &CS : this->ICF->getCallsFromWithin(Func)) {
      for (const auto &BeforeCS : this->ICF->getPredsOf(CS)) {
        auto LookupResults = this->JumpFn->forwardLookup(Fact, BeforeCS);
        if (!LookupResults) {
          continue;
        }
        for (size_t I = 0; I < LookupResults->get().size(); ++I) {
          auto Entry = LookupResults->get()[I];
          d_t dPrime = Entry.first;
          auto fPrime = Entry.second;
          n_t SP = Stmt;
          l_t Val = seedVal(SP, Fact);
          INC_COUNTER("Value Propagation", 1, Full);
          this->propagateValue(CS, dPrime, fPrime.computeTarget(Val));
        }
      }
    }
  }

  std::vector<n_t> getAllValueComputationNodes() const {
    std::vector<n_t> Ret;
    // TODO: Reserve

    for (const auto &Fun : this->ICF->getAllFunctions()) {
      for (const auto &Inst : this->ICF->getAllInstructionsOf(Fun)) {
        Ret.push_back(Inst);
      }
    }
    return Ret;
  }

  l_t seedVal(n_t NHashN, d_t NHashD) {
    if (SeedValues.contains(NHashN, NHashD)) {
      return SeedValues.get(NHashN, NHashD);
    }
    return this->IDEProblem.topElement();
  }

  void setSeedVal(n_t NHashN, d_t NHashD, l_t L) {
    SeedValues.insert(std::move(NHashN), std::move(NHashD), std::move(L));
  }

  // -- Data members

  std::vector<std::pair<PathEdge<n_t, d_t>, EdgeFunction<l_t>>> WorkList;
  Table<n_t, d_t, l_t> SeedValues;
};

template <typename Problem, typename ICF>
IDESolver(Problem &, ICF *, PropagateOntoStrategy)
    -> IDESolver<typename Problem::ProblemAnalysisDomain,
                 typename Problem::container_type, PropagateOntoStrategy>;

template <typename AnalysisDomainTy, typename Container>
OwningSolverResults<typename AnalysisDomainTy::n_t,
                    typename AnalysisDomainTy::d_t,
                    typename AnalysisDomainTy::l_t>
solveIDEProblem(IDETabulationProblem<AnalysisDomainTy, Container> &Problem,
                const typename AnalysisDomainTy::i_t &ICF,
                PropagateOntoStrategy Strategy) {
  IDESolver Solver(Problem, &ICF, Strategy);
  Solver.solve();
  return Solver.consumeSolverResults();
}

} // namespace psr

#endif