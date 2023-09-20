/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_HELPERANALYSISCONFIG_H
#define PHASAR_PHASARLLVM_HELPERANALYSISCONFIG_H

#include "phasar/ControlFlow/CallGraphAnalysisType.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Utils/Soundness.h"

#include "nlohmann/json.hpp"

#include <optional>

namespace psr {
struct HelperAnalysisConfig {
  std::optional<nlohmann::json> PrecomputedPTS = std::nullopt;
  std::optional<nlohmann::json> PrecomputedCG = std::nullopt;
  AliasAnalysisType PTATy = AliasAnalysisType::CFLAnders;
  CallGraphAnalysisType CGTy = CallGraphAnalysisType::OTF;
  Soundness SoundnessLevel = Soundness::Soundy;
  bool AutoGlobalSupport = true;
  bool AllowLazyPTS = true;

  HelperAnalysisConfig &&withCGType(CallGraphAnalysisType CGTy) &&noexcept {
    this->CGTy = CGTy;
    return std::move(*this);
  }
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_HELPERANALYSISCONFIG_H
