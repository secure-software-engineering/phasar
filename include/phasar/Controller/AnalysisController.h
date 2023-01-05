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

#include "phasar/AnalysisStrategy/Strategies.h"
#include "phasar/Controller/AnalysisControllerEmitterOptions.h"
#include "phasar/DataFlow/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/DataFlow/IfdsIde/Solver/IDESolver.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/DataFlow/IfdsIde/SolverResults.h"
#include "phasar/DataFlow/Mono/Solver/InterMonoSolver.h"
#include "phasar/DataFlow/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/IO.h"
#include "phasar/Utils/Soundness.h"

#include <set>
#include <string>
#include <vector>

namespace psr {

class AnalysisController {
private:
  HelperAnalyses &HA;
  std::vector<DataFlowAnalysisType> DataFlowAnalyses;
  std::vector<std::string> AnalysisConfigs;
  std::vector<std::string> EntryPoints;
  [[maybe_unused]] AnalysisStrategy Strategy;
  AnalysisControllerEmitterOptions EmitterOptions =
      AnalysisControllerEmitterOptions::None;
  std::string ProjectID;
  std::filesystem::path ResultDirectory;
  IFDSIDESolverConfig SolverConfig;

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

  template <typename ProblemTy>
  void executeIntraMonoAnalysis(ProblemTy &Problem) {
    IntraMonoSolver Solver(Problem);
    Solver.solve();
    emitRequestedDataFlowResults(Solver);
  }

  template <typename ProblemTy>
  void executeInterMonoAnalysis(ProblemTy &Problem) {
    InterMonoSolver_P<ProblemTy, 3> Solver(Problem);
    Solver.solve();
    emitRequestedDataFlowResults(Solver);
  }

  template <typename ProblemTy> void executeIFDSAnalysis(ProblemTy &Problem) {
    IFDSSolver Solver(Problem, &HA.getICFG());
    Solver.solve();
    emitRequestedDataFlowResults(Solver);
  }

  template <typename ProblemTy> void executeIDEAnalysis(ProblemTy &Problem) {
    IDESolver Solver(Problem, &HA.getICFG());
    Solver.solve();
    emitRequestedDataFlowResults(Solver);
  }

  TaintConfig makeTaintConfig();

  template <typename T> void emitRequestedDataFlowResults(T &Solver) {
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTextReport) {
      if (!ResultDirectory.empty()) {
        if (auto OFS =
                openFileStream(ResultDirectory.string() + "/psr-report.txt")) {
          Solver.emitTextReport(*OFS);
        }
      } else {
        Solver.emitTextReport(llvm::outs());
      }
    }
    if (EmitterOptions &
        AnalysisControllerEmitterOptions::EmitGraphicalReport) {
      if (!ResultDirectory.empty()) {
        if (auto OFS =
                openFileStream(ResultDirectory.string() + "/psr-report.html")) {
          Solver.emitGraphicalReport(*OFS);
        }
      } else {
        Solver.emitGraphicalReport(llvm::outs());
      }
    }
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitRawResults) {
      if (!ResultDirectory.empty()) {
        if (auto OFS = openFileStream(ResultDirectory.string() +
                                      "/psr-raw-results.txt")) {
          Solver.dumpResults(*OFS);
        }
      } else {
        Solver.dumpResults(llvm::outs());
      }
    }
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitESGAsDot) {
      llvm::outs()
          << "Front-end support for 'EmitESGAsDot' to be implemented\n";
    }
  }

public:
  explicit AnalysisController(
      HelperAnalyses &HA, std::vector<DataFlowAnalysisType> DataFlowAnalyses,
      std::vector<std::string> AnalysisConfigs,
      std::vector<std::string> EntryPoints, AnalysisStrategy Strategy,
      AnalysisControllerEmitterOptions EmitterOptions,
      IFDSIDESolverConfig SolverConfig,
      const std::string &ProjectID = "default-phasar-project",
      const std::string &OutDirectory = "");

  ~AnalysisController() = default;

  AnalysisController(const AnalysisController &) = delete;
  AnalysisController(AnalysisController &&) = delete;
  AnalysisController &operator=(const AnalysisController &) = delete;
  AnalysisController &operator=(const AnalysisController &&) = delete;

  void executeAs(AnalysisStrategy Strategy);

  static constexpr bool
  needsToEmitPTA(AnalysisControllerEmitterOptions EmitterOptions) {
    return (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText);
  }
};

} // namespace psr

#endif
