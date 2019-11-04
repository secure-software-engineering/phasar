/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_ANALYSISSTRATEGY_WHOLEPROGRAMANALYSIS_H_
#define PHASAR_PHASARLLVM_ANALYSISSTRATEGY_WHOLEPROGRAMANALYSIS_H_

#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <type_traits>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/AnalysisSetup.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h>

namespace psr {

template <typename Solver, typename ProblemDescription,
          typename Setup = psr::DefaultAnalysisSetup>
class WholeProgramAnalysis {
  static_assert(
      std::is_base_of_v<typename Solver::ProblemTy, ProblemDescription>,
      "Problem description does not match solver type!");
  static_assert(std::is_base_of_v<psr::AnalysisSetup, Setup>,
                "Setup is not a valid analysis setup!");

private:
  ProjectIRDB &IRDB;
  std::unique_ptr<typename Setup::TypeHierarchyTy> TypeHierarchy;
  std::unique_ptr<typename Setup::CallGraphAnalysisTy> CallGraph;
  std::unique_ptr<typename Setup::PointerAnalysisTy> PointerInfo;
  // ProblemDescription ProblemDesc;
  // Solver DataFlowSolver;
  std::vector<std::string> EntryPoints;

public:
  WholeProgramAnalysis(ProjectIRDB &IRDB,
                       std::initializer_list<std::string> EntryPoints = {},
                       typename Setup::PointerAnalysisTy *PointerInfo = nullptr,
                       typename Setup::CallGraphAnalysisTy *CallGraph = nullptr,
                       typename Setup::TypeHierarchyTy *TypeHierarchy = nullptr)
      : IRDB(IRDB), EntryPoints(EntryPoints) {}
  // : IRDB(IRDB),
  //   PointerInfo((PointerInfo == nullptr
  //                    ? std::make_unique<typename Setup::PointerAnalysisTy>()
  //                    : PointerInfo)),
  //   TypeHierarchy(TypeHierarchy == nullptr
  //                     ? std::make_unique<typename
  //                     Setup::TypeHierarchyTy>(IRDB) : TypeHierarchy),
  //   CallGraph(CallGraph == nullptr
  //                 ? std::make_unique<typename
  //                 Setup::CallGraphAnalysisTy>(TypeHierarchy, IRDB) :
  //                 CallGraph) {}
  // ProblemDesc()
  //, DataFlowSolver(ProblemDesc)
  // {}

  void solve() {
    // DataFlowSolver.solve();
  }

  void operator()() { solve(); }

  void dumpResults(std::ostream &OS = std::cout) {}

  void emitTextualReport(std::ostream &OS) {}

  void emitGraphicalReport() {}

  void releaseAllHelperAnalysis() {
    releasePointerInformation();
    releaseCallGraph();
    releaseTypeHierarchy();
  }

  typename Setup::TypeHierarchyTy *releasePointerInformation() {
    return PointerInfo.release();
  }

  typename Setup::CallGraphAnalysisTy *releaseCallGraph() {
    return CallGraph.release();
  }

  typename Setup::TypeHierarchyTy *releaseTypeHierarchy() {
    return TypeHierarchy.release();
  }
};

} // namespace psr

#endif
