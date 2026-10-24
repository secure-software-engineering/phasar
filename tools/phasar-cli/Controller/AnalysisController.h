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

#include "llvm/ADT/SmallString.h"

#include "AnalysisControllerEmitterOptions.h"

namespace psr {

struct AnalysisController {
  HelperAnalyses HA;
  std::vector<DataFlowAnalysisType> DataFlowAnalyses;
  std::vector<std::string> AnalysisConfigs;
  [[maybe_unused]] AnalysisStrategy Strategy{};
  AnalysisControllerEmitterOptions EmitterOptions =
      AnalysisControllerEmitterOptions::None;
  IFDSIDESolverConfig SolverConfig{};
  llvm::SmallString<128> ProjectID;
  llvm::SmallString<128> ResultDirectory;

  static constexpr bool
  needsToEmitPTA(AnalysisControllerEmitterOptions EmitterOptions) {
    return (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsDot) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsJson) ||
           (EmitterOptions & AnalysisControllerEmitterOptions::EmitPTAAsText);
  }

  void emitRequestedHelperAnalysisResults();
  void run();

  [[nodiscard]] const auto &getEntryPoints() const noexcept {
    return HA.getEntryPoints();
  }
};

} // namespace psr

#endif
