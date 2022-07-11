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

#include <iosfwd>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/AnalysisSetup.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"

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

private:
  using TypeHierarchyTy = typename Setup::TypeHierarchyTy;
  using PointerAnalysisTy = typename Setup::PointerAnalysisTy;
  using CallGraphAnalysisTy = typename Setup::CallGraphAnalysisTy;
  using ConfigurationTy = typename ProblemDescription::ConfigurationTy;

  ProjectIRDB &IRDB;
  std::unique_ptr<TypeHierarchyTy> TypeHierarchy;
  std::unique_ptr<PointerAnalysisTy> PointerInfo;
  std::unique_ptr<CallGraphAnalysisTy> CallGraph;
  std::set<std::string> EntryPoints;
  ConfigurationTy *Config = nullptr;
  bool OwnsConfig = false;
  std::string ConfigPath;
  ProblemDescription ProblemDesc;
  Solver DataFlowSolver;

public:
  WholeProgramAnalysis(IFDSIDESolverConfig SolverConfig, ProjectIRDB &IRDB,
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
                            IRDB, CallGraphAnalysisType::OTF, EntryPoints,
                            this->TypeHierarchy.get(), this->PointerInfo.get())
                      : std::unique_ptr<CallGraphAnalysisTy>(CallGraph)),
        EntryPoints(EntryPoints),
        ProblemDesc(&IRDB, TypeHierarchy, CallGraph, PointerInfo, EntryPoints),
        DataFlowSolver(ProblemDesc) {
    if constexpr (has_setIFDSIDESolverConfig_v<ProblemDescription>) {
      ProblemDesc.setIFDSIDESolverConfig(SolverConfig);
    }
  }

  template <typename T = ProblemDescription,
            typename = typename std::enable_if_t<!std::is_same_v<
                typename T::ConfigurationTy, HasNoConfigurationType>>>
  WholeProgramAnalysis(IFDSIDESolverConfig SolverConfig, ProjectIRDB &IRDB,
                       ConfigurationTy *Config,
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
                            IRDB, CallGraphAnalysisType::OTF, EntryPoints,
                            this->TypeHierarchy.get(), this->PointerInfo.get())
                      : std::unique_ptr<CallGraphAnalysisTy>(CallGraph)),
        EntryPoints(EntryPoints), Config(Config),
        ProblemDesc(&IRDB, TypeHierarchy, CallGraph, PointerInfo, *Config,
                    EntryPoints),
        DataFlowSolver(ProblemDesc) {
    if constexpr (has_setIFDSIDESolverConfig_v<ProblemDescription>) {
      ProblemDesc.setIFDSIDESolverConfig(SolverConfig);
    }
  }

  template <typename T = ProblemDescription,
            typename = typename std::enable_if_t<!std::is_same_v<
                typename T::ConfigurationTy, HasNoConfigurationType>>>
  WholeProgramAnalysis(IFDSIDESolverConfig SolverConfig, ProjectIRDB &IRDB,
                       std::string ConfigPath,
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
                            IRDB, CallGraphAnalysisType::OTF, EntryPoints,
                            this->TypeHierarchy.get(), this->PointerInfo.get())
                      : std::unique_ptr<CallGraphAnalysisTy>(CallGraph)),
        EntryPoints(EntryPoints), Config(new ConfigurationTy(ConfigPath)),
        OwnsConfig(true), ConfigPath(ConfigPath),
        ProblemDesc(&IRDB, TypeHierarchy, CallGraph, PointerInfo, *Config,
                    EntryPoints),
        DataFlowSolver(ProblemDesc) {
    if constexpr (has_setIFDSIDESolverConfig_v<ProblemDescription>) {
      ProblemDesc.setIFDSIDESolverConfig(SolverConfig);
    }
  }

  WholeProgramAnalysis(const WholeProgramAnalysis &) = delete;
  WholeProgramAnalysis(WholeProgramAnalysis &&) = delete;
  WholeProgramAnalysis &operator=(WholeProgramAnalysis &) = delete;
  WholeProgramAnalysis &operator=(WholeProgramAnalysis &&) = delete;

  ~WholeProgramAnalysis() {
    if (OwnsConfig) {
      delete Config;
      Config = nullptr;
    }
  }

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

  void emitESG(llvm::raw_ostream &OS = llvm::outs()) {
    // if (std::is_base_of_v<typename Solver::ProblemTy, ProblemDescription>) {
    //   DataFlowSolver.emitESGAsDot(OS);
    // }
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
};

} // namespace psr

#endif
