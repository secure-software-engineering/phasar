/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSIS_CONTROLLER_H_
#define PHASAR_CONTROLLER_ANALYSIS_CONTROLLER_H_

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "boost/filesystem.hpp"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/AnalysisStrategy/Strategies.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/SoundnessFlag.h"

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
  EmitCGAsText = (1 << 8),
  EmitCGAsDot = (1 << 9),
  EmitCGAsJson = (1 << 10),
  EmitPTAAsText = (1 << 11),
  EmitPTAAsDot = (1 << 12),
  EmitPTAAsJson = (1 << 13),
};

class AnalysisController {
private:
  ProjectIRDB &IRDB;
  LLVMTypeHierarchy TH;
  LLVMPointsToSet PT;
  LLVMBasedICFG ICF;
  std::vector<DataFlowAnalysisKind> DataFlowAnalyses;
  std::vector<std::string> AnalysisConfigs;
  std::set<std::string> EntryPoints;
  [[maybe_unused]] AnalysisStrategy Strategy;
  AnalysisControllerEmitterOptions EmitterOptions =
      AnalysisControllerEmitterOptions::None;
  std::string ProjectID;
  std::string OutDirectory;
  boost::filesystem::path ResultDirectory;
  [[maybe_unused]] SoundnessFlag SF;

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

  template <typename T> void emitRequestedDataFlowResults(T &WPA) {
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitTextReport) {
      if (!ResultDirectory.empty()) {
        std::ofstream OFS(ResultDirectory.string() + "/psr-report.txt");
        WPA.emitTextReport(OFS);
      } else {
        WPA.emitTextReport(std::cout);
      }
    }
    if (EmitterOptions &
        AnalysisControllerEmitterOptions::EmitGraphicalReport) {
      if (!ResultDirectory.empty()) {
        std::ofstream OFS(ResultDirectory.string() + "/psr-report.html");
        WPA.emitGraphicalReport(OFS);
      } else {
        WPA.emitGraphicalReport(std::cout);
      }
    }
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitRawResults) {
      if (!ResultDirectory.empty()) {
        std::ofstream OFS(ResultDirectory.string() + "/psr-raw-results.txt");
        WPA.dumpResults(OFS);
      } else {
        WPA.dumpResults(std::cout);
      }
    }
    if (EmitterOptions & AnalysisControllerEmitterOptions::EmitESGAsDot) {
      std::cout << "Front-end support for 'EmitESGAsDot' to be implemented\n";
    }
  }

public:
  AnalysisController(ProjectIRDB &IRDB,
                     std::vector<DataFlowAnalysisKind> DataFlowAnalyses,
                     std::vector<std::string> AnalysisConfigs,
                     PointerAnalysisType PTATy, CallGraphAnalysisType CGTy,
                     SoundnessFlag SF, const std::set<std::string> &EntryPoints,
                     AnalysisStrategy Strategy,
                     AnalysisControllerEmitterOptions EmitterOptions,
                     const std::string &ProjectID = "default-phasar-project",
                     const std::string &OutDirectory = "");

  ~AnalysisController() = default;

  AnalysisController(const AnalysisController &) = delete;

  AnalysisController(AnalysisController &&) = delete;

  void executeAs(AnalysisStrategy Strategy);
};

} // namespace psr

#endif
