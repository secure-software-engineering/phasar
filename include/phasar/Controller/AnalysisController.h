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
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"

#include <filesystem>
namespace psr {

class AnalysisController {
public:
  struct ControllerData {
    HelperAnalyses *HA{};
    std::vector<DataFlowAnalysisType> DataFlowAnalyses;
    std::vector<std::string> AnalysisConfigs;
    std::vector<std::string> EntryPoints;
    [[maybe_unused]] AnalysisStrategy Strategy;
    AnalysisControllerEmitterOptions EmitterOptions =
        AnalysisControllerEmitterOptions::None;
    std::string ProjectID;
    std::filesystem::path ResultDirectory;
    IFDSIDESolverConfig SolverConfig{};
  };

  explicit AnalysisController(
      HelperAnalyses &HA, std::vector<DataFlowAnalysisType> DataFlowAnalyses,
      std::vector<std::string> AnalysisConfigs,
      std::vector<std::string> EntryPoints, AnalysisStrategy Strategy,
      AnalysisControllerEmitterOptions EmitterOptions,
      IFDSIDESolverConfig SolverConfig,
      std::string ProjectID = "default-phasar-project",
      std::string OutDirectory = "");

  static constexpr bool
  needsToEmitPTA(AnalysisControllerEmitterOptions EmitterOptions) {
    return (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText);
  }

private:
  ControllerData Data;
};

} // namespace psr

#endif
