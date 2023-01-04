/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_ANALYSISSTRATEGY_HELPERANALYSISCONFIG_H
#define PHASAR_PHASARLLVM_ANALYSISSTRATEGY_HELPERANALYSISCONFIG_H

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"
#include "phasar/Pointer/PointerAnalysisType.h"

#include "nlohmann/json.hpp"
#include "phasar/Utils/Soundness.h"

#include <optional>

namespace psr {
struct HelperAnalysisConfig {
  std::optional<nlohmann::json> PrecomputedPTS = std::nullopt;
  PointerAnalysisType PTATy = PointerAnalysisType::CFLAnders;
  CallGraphAnalysisType CGTy = CallGraphAnalysisType::OTF;
  Soundness SoundnessLevel = Soundness::Soundy;
  bool AutoGlobalSupport = true;
  bool AllowLazyPTS = true;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_ANALYSISSTRATEGY_HELPERANALYSISCONFIG_H
