/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSISCONTROLLER_H
#define PHASAR_CONTROLLER_ANALYSISCONTROLLER_H

#include <set>
#include <string>
#include <vector>

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/WholeProgramAnalysis.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/InterMonoSolver.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/Soundness.h"

namespace psr {

enum class AnalysisControllerEmitterOptions : uint32_t {
  None = 0,
  EmitIR = (1 << 0),
  EmitRawResults = (1 << 1),
  EmitTextReport = (1 << 2),
  EmitGraphicalReport = (1 << 3),
  EmitESGAsDot = (1 << 4),
  EmitTHAsText = (1 << 5),
  EmitTHAsDot = (1 << 6),
  EmitTHAsJson = (1 << 7),
  // EmitCGAsText = (1 << 8),
  EmitCGAsDot = (1 << 9),
  // EmitCGAsJson = (1 << 10),
  EmitPTAAsText = (1 << 11),
  EmitPTAAsDot = (1 << 12),
  EmitPTAAsJson = (1 << 13),
  EmitStatisticsAsJson = (1 << 14),
};

class AnalysisController {
private:
  ProjectIRDB &IRDB;
  LLVMTypeHierarchy TH;
  LLVMPointsToSet PT;
  LLVMBasedICFG ICF;
  std::vector<DataFlowAnalysisType> DataFlowAnalyses;
  std::vector<std::string> AnalysisConfigs;
  std::vector<std::string> EntryPoints;
  [[maybe_unused]] AnalysisStrategy Strategy;
  AnalysisControllerEmitterOptions EmitterOptions =
      AnalysisControllerEmitterOptions::None;
  std::string ProjectID;
  std::string OutDirectory;
  std::filesystem::path ResultDirectory;
  IFDSIDESolverConfig SolverConfig;
  [[maybe_unused]] Soundness SoundnessLevel;
  [[maybe_unused]] bool AutoGlobalSupport;

  ///
  /// \brief The maximum length of the CallStrings used in the InterMonoSolver
  ///
  static const unsigned K = 3;

  void executeDemandDriven();

  void executeIncremental();

  void executeModuleWise();

  void executeVariational();

  void executeWholeProgram();

  void emitRequestedHelperAnalysisResults();

  void executeIFDSUninitVar();
  void executeIFDSConst();
  void executeIFDSTaint();
  void executeIFDSType();
  void executeIFDSSolverTest();
  void executeIFDSFieldSensTaint();
  void executeIDEXTaint();
  void executeIDEOpenSSLTS();
  void executeIDECSTDIOTS();
  void executeIDELinearConst();
  void executeIDESolverTest();
  void executeIDEIIA();
  void executeIntraMonoFullConstant();
  void executeIntraMonoSolverTest();
  void executeInterMonoSolverTest();
  void executeInterMonoTaint();

  template <typename AnalysisTy, bool WithConfig = false>
  void executeIntraMonoAnalysis() {
    executeAnalysis<IntraMonoSolver_P<AnalysisTy>, AnalysisTy, WithConfig>();
  }

  template <typename AnalysisTy, bool WithConfig = false>
  void executeInterMonoAnalysis() {
    executeAnalysis<InterMonoSolver_P<AnalysisTy, 3>, AnalysisTy, WithConfig>();
  }

  template <typename AnalysisTy, bool WithConfig = false>
  void executeIFDSAnalysis() {
    executeAnalysis<IFDSSolver_P<AnalysisTy>, AnalysisTy, WithConfig>();
  }

  template <typename AnalysisTy, bool WithConfig = false>
  void executeIDEAnalysis() {
    executeAnalysis<IDESolver_P<AnalysisTy>, AnalysisTy, WithConfig>();
  }

  template <class Solver_P, typename AnalysisTy, bool WithConfig>
  void executeAnalysis() {
    if constexpr (WithConfig) {
      std::string AnalysisConfigPath =
          !AnalysisConfigs.empty() ? AnalysisConfigs[0] : "";
      auto Config =
          !AnalysisConfigPath.empty()
              ? TaintConfig(IRDB, parseTaintConfig(AnalysisConfigPath))
              : TaintConfig(IRDB);
      WholeProgramAnalysis<Solver_P, AnalysisTy> WPA(
          SolverConfig, IRDB, &Config, EntryPoints, &PT, &ICF, &TH);
      WPA.solve();
      emitRequestedDataFlowResults(WPA);
      WPA.releaseAllHelperAnalyses();
    } else {
      WholeProgramAnalysis<Solver_P, AnalysisTy> WPA(
          SolverConfig, IRDB, EntryPoints, &PT, &ICF, &TH);
      WPA.solve();
      emitRequestedDataFlowResults(WPA);
      WPA.releaseAllHelperAnalyses();
    }
  }

  std::unique_ptr<llvm::raw_fd_ostream>
  openFileStream(llvm::StringRef Filename);

  template <typename T> void emitRequestedDataFlowResults(T &WPA) {
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTextReport) {
      if (!ResultDirectory.empty()) {
        if (auto OFS = openFileStream("/psr-report.txt")) {
          WPA.emitTextReport(*OFS);
        }
      } else {
        WPA.emitTextReport(llvm::outs());
      }
    }
    if (EmitterOptions &
        AnalysisControllerEmitterOptions::EmitGraphicalReport) {
      if (!ResultDirectory.empty()) {
        if (auto OFS = openFileStream("/psr-report.html")) {
          WPA.emitGraphicalReport(*OFS);
        }
      } else {
        WPA.emitGraphicalReport(llvm::outs());
      }
    }
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitRawResults) {
      if (!ResultDirectory.empty()) {
        if (auto OFS = openFileStream("/psr-raw-results.txt")) {
          WPA.dumpResults(*OFS);
        }
      } else {
        WPA.dumpResults(llvm::outs());
      }
    }
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitESGAsDot) {
      llvm::outs()
          << "Front-end support for 'EmitESGAsDot' to be implemented\n";
    }
  }

public:
  AnalysisController(ProjectIRDB &IRDB,
                     std::vector<DataFlowAnalysisType> DataFlowAnalyses,
                     std::vector<std::string> AnalysisConfigs,
                     PointerAnalysisType PTATy, CallGraphAnalysisType CGTy,
                     Soundness SoundnessLevel, bool AutoGlobalSupport,
                     std::vector<std::string> EntryPoints,
                     AnalysisStrategy Strategy,
                     AnalysisControllerEmitterOptions EmitterOptions,
                     IFDSIDESolverConfig SolverConfig,
                     const std::string &ProjectID = "default-phasar-project",
                     const std::string &OutDirectory = "",
                     const nlohmann::json &PrecomputedPointsToInfo = {});

  ~AnalysisController() = default;

  AnalysisController(const AnalysisController &) = delete;
  AnalysisController(AnalysisController &&) = delete;
  AnalysisController &operator=(const AnalysisController &) = delete;
  AnalysisController &operator=(const AnalysisController &&) = delete;

  void executeAs(AnalysisStrategy Strategy);
};

} // namespace psr

#endif
