/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_ANALYSISSTRATEGY_VARIATIONALANALYSIS_H
#define PHASAR_ANALYSISSTRATEGY_VARIATIONALANALYSIS_H

#include "phasar/AnalysisStrategy/AnalysisSetup.h"

#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <set>
#include <string>
#include <type_traits>

namespace psr {

template <typename Solver, typename ProblemDescription,
          typename Setup = psr::DefaultAnalysisSetup>
class VariationalAnalysis {
  static_assert(
      std::is_base_of_v<typename Solver::ProblemType, ProblemDescription>,
      "ProblemDesciption does not match SolverType");
  // Check if the setup is a valid analysis setup
  static_assert(std::is_base_of_v<psr::AnalysisSetup, Setup>,
                "Setup is not a valid analysis setup!");

private:
  using TypeHierarchyTy = typename Setup::TypeHierarchyTy;
  using PointerAnalysisTy = typename Setup::PointerAnalysisTy;
  using CallGraphAnalysisTy = typename Setup::CallGraphAnalysisTy;
  using ProjectIRDBTy = typename Setup::ProjectIRDBTy;
  using ConfigurationTy = typename ProblemDescription::ConfigurationTy;

  ProjectIRDBTy &IRDB;
  std::unique_ptr<TypeHierarchyTy> TypeHierarchy;
  std::unique_ptr<PointerAnalysisTy> PointerInfo;
  std::unique_ptr<CallGraphAnalysisTy> CallGraph;
  std::set<std::string> EntryPoints;
  std::unique_ptr<ConfigurationTy> Config;
  std::string ConfigPath;
  ProblemDescription ProblemDesc;
  Solver DataFlowSolver;

public:
  VariationalAnalysis(ProjectIRDBTy &IRDB,
                      std::set<std::string> EntryPoints = {},
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
                            IRDB, CallGraphAnalysisTy::OTF, EntryPoints,
                            this->TypeHierarchy.get(), this->PointerInfo.get())
                      : std::unique_ptr<CallGraphAnalysisTy>(CallGraph)),
        EntryPoints(EntryPoints),
        ProblemDesc(&IRDB, TypeHierarchy, CallGraph, PointerInfo, EntryPoints),
        DataFlowSolver(ProblemDesc) {}

  // template <typename T = ProblemDescription,
  //           typename = typename std::enable_if_t<!std::is_same_v<
  //               typename T::ConfigurationTy, HasNoConfigurationType>>>
  // WholeProgramAnalysis(ProjectIRDB &IRDB, ConfigurationTy *Config,
  //                      std::set<std::string> EntryPoints = {},
  //                      PointerAnalysisTy *PointerInfo = nullptr,
  //                      CallGraphAnalysisTy *CallGraph = nullptr,
  //                      TypeHierarchyTy *TypeHierarchy = nullptr)
  //     : IRDB(IRDB),
  //       TypeHierarchy(TypeHierarchy == nullptr
  //                         ? std::make_unique<TypeHierarchyTy>(IRDB)
  //                         : std::unique_ptr<TypeHierarchyTy>(TypeHierarchy)),
  //       PointerInfo(PointerInfo == nullptr
  //                       ? std::make_unique<PointerAnalysisTy>(IRDB)
  //                       : std::unique_ptr<PointerAnalysisTy>(PointerInfo)),
  //       CallGraph(CallGraph == nullptr
  //                     ? std::make_unique<CallGraphAnalysisTy>(
  //                           IRDB, CallGraphAnalysisType::OTF, EntryPoints,
  //                           this->TypeHierarchy.get(),
  //                           this->PointerInfo.get())
  //                     : std::unique_ptr<CallGraphAnalysisTy>(CallGraph)),
  //       EntryPoints(EntryPoints),
  //       Config(std::unique_ptr<ConfigurationTy>(Config)), ConfigPath(""),
  //       ProblemDesc(&IRDB, TypeHierarchy, CallGraph, PointerInfo, *Config,
  //                   EntryPoints),
  //       DataFlowSolver(ProblemDesc) {}

  // template <typename T = ProblemDescription,
  //           typename = typename std::enable_if_t<!std::is_same_v<
  //               typename T::ConfigurationTy, HasNoConfigurationType>>>
  // WholeProgramAnalysis(ProjectIRDB &IRDB, std::string ConfigPath,
  //                      std::set<std::string> EntryPoints = {},
  //                      PointerAnalysisTy *PointerInfo = nullptr,
  //                      CallGraphAnalysisTy *CallGraph = nullptr,
  //                      TypeHierarchyTy *TypeHierarchy = nullptr)
  //     : IRDB(IRDB),
  //       TypeHierarchy(TypeHierarchy == nullptr
  //                         ? std::make_unique<TypeHierarchyTy>(IRDB)
  //                         : std::unique_ptr<TypeHierarchyTy>(TypeHierarchy)),
  //       PointerInfo(PointerInfo == nullptr
  //                       ? std::make_unique<PointerAnalysisTy>(IRDB)
  //                       : std::unique_ptr<PointerAnalysisTy>(PointerInfo)),
  //       CallGraph(CallGraph == nullptr
  //                     ? std::make_unique<CallGraphAnalysisTy>(
  //                           IRDB, CallGraphAnalysisType::OTF, EntryPoints,
  //                           this->TypeHierarchy.get(),
  //                           this->PointerInfo.get())
  //                     : std::unique_ptr<CallGraphAnalysisTy>(CallGraph)),
  //       EntryPoints(EntryPoints),
  //       Config(std::make_unique<ConfigurationTy>(ConfigPath)),
  //       ConfigPath(ConfigPath), ProblemDesc(&IRDB, TypeHierarchy, CallGraph,
  //                                           PointerInfo, *Config,
  //                                           EntryPoints),
  //       DataFlowSolver(ProblemDesc) {}

  void solve() { DataFlowSolver.solve(); }

  void operator()() { solve(); }

  void dumpResults(llvm::raw_ostream &OS = llvm::outs()) {
    DataFlowSolver.dumpResults(OS);
  }

  void emitTextReport(llvm::raw_ostream &OS = llvm::outs()) {
    DataFlowSolver.emitTextReport(OS);
  }

  void emitGraphicalReport(llvm::raw_ostream &OS = llvm::outs()) {
    DataFlowSolver.emitGraphicalReport(OS);
  }

  void releaseAllHelperAnalyses() {
    releasePointerInformation();
    releaseCallGraph();
    releaseTypeHierarchy();
  }

  PointerAnalysisTy *releasePointerInformation() {
    return PointerInfo.release();
  }

  CallGraphAnalysisTy *releaseCallGraph() { return CallGraph.release(); }

  TypeHierarchyTy *releaseTypeHierarchy() { return TypeHierarchy.release(); }

  ConfigurationTy *releaseConfiguration() { return Config.release(); }
};

} // namespace psr

#endif
