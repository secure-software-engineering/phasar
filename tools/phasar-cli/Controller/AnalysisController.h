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
#include "phasar/DataFlow/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"

#include "AnalysisControllerEmitterOptions.h"

#include <filesystem>
namespace psr {

struct AnalysisController {
  HelperAnalyses *HA{};
  std::vector<DataFlowAnalysisType> DataFlowAnalyses;
  std::vector<std::string> AnalysisConfigs;
  std::vector<std::string> EntryPoints;
  [[maybe_unused]] AnalysisStrategy Strategy{};
  AnalysisControllerEmitterOptions EmitterOptions =
      AnalysisControllerEmitterOptions::None;
  IFDSIDESolverConfig SolverConfig{};
  std::string ProjectID = "default-phasar-project";
  std::filesystem::path ResultDirectory;

  static constexpr bool
  needsToEmitPTA(AnalysisControllerEmitterOptions EmitterOptions) {
    return (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText);
  }

  void emitRequestedHelperAnalysisResults();
  void run();
};

} // namespace psr

#endif
