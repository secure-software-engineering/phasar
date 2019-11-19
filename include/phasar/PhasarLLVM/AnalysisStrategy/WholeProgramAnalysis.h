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

namespace psr {

template <typename Solver, typename ProblemDescription,
          typename Setup = psr::DefaultAnalysisSetup>
class WholeProgramAnalysis {
  // Check if the solver is able to solve the given problem description
  static_assert(
      std::is_base_of_v<typename Solver::ProblemTy, ProblemDescription>,
      "Problem description does not match solver type!");
  // Check if the setup is a valid analysis setup
  static_assert(std::is_base_of_v<psr::AnalysisSetup, Setup>,
                "Setup is not a valid analysis setup!");
  // Check if the configuration is a valid for the given problem
  // static_assert(
  //     std::is_same_v<typename ProblemDescription::ConfigurationTy, Configuration>,
  //     "Configuration is not valid for the given problem description!");

private:
  using TypeHierarchyTy = typename Setup::TypeHierarchyTy;
  using PointerAnalysisTy = typename Setup::PointerAnalysisTy;
  using CallGraphAnalysisTy = typename Setup::CallGraphAnalysisTy;
  using ConfigurationTy = typename ProblemDescription::ConfigurationTy;

  ProjectIRDB &IRDB;
  std::unique_ptr<TypeHierarchyTy> TypeHierarchy;
  std::unique_ptr<PointerAnalysisTy> PointerInfo;
  std::unique_ptr<CallGraphAnalysisTy> CallGraph;
  std::unique_ptr<ConfigurationTy> Config;
  ProblemDescription ProblemDesc;
  Solver DataFlowSolver;
  std::vector<std::string> EntryPoints;

public:
  WholeProgramAnalysis(ProjectIRDB &IRDB,
                       std::initializer_list<std::string> EntryPoints = {},
                       PointerAnalysisTy *PointerInfo = nullptr,
                       CallGraphAnalysisTy *CallGraph = nullptr,
                       TypeHierarchyTy *TypeHierarchy = nullptr)
      : IRDB(IRDB),
        TypeHierarchy(TypeHierarchy == nullptr
                          ? std::make_unique<TypeHierarchyTy>(IRDB)
                          : std::unique_ptr<TypeHierarchyTy>(TypeHierarchy)),
        PointerInfo(PointerInfo == nullptr
                        ? std::make_unique<PointerAnalysisTy>(IRDB)
                        : std::unique_ptr<PointerAnalysisTy>(PointerInfo)),
        CallGraph(CallGraph == nullptr
                      ? std::make_unique<CallGraphAnalysisTy>(
                            *TypeHierarchy, IRDB, CallGraphAnalysisType::OTF,
                            EntryPoints)
                      : std::unique_ptr<CallGraphAnalysisTy>(CallGraph)),
        ProblemDesc(*CallGraph), DataFlowSolver(ProblemDesc),
        EntryPoints(EntryPoints) {}

  void solve() { DataFlowSolver.solve(); }

  void operator()() { solve(); }

  void dumpResults(std::ostream &OS = std::cout) {}

  void emitTextualReport(std::ostream &OS) {}

  void emitGraphicalReport() {}

  void releaseAllHelperAnalyses() {
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
