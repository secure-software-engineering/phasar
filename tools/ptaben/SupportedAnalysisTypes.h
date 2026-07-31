#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/StringRef.h"

#include <cstddef>

namespace psr::ptaben {
enum class SupportedAnalysisTypes { // NOLINT
#define PSR_PTABEN_SUPPORTED_ANALYSIS_TYPES(NAME, CMD, CSV) NAME,
#include "SupportedAnalysisTypes.def"
};

constexpr size_t NumSupportedAnalysisTypes = 0
#define PSR_PTABEN_SUPPORTED_ANALYSIS_TYPES(NAME, CMD, CSV) +1 // NOLINT
#include "SupportedAnalysisTypes.def"
    ;

constexpr SupportedAnalysisTypes AllSupportedAnalysisTypes[]{
#define PSR_PTABEN_SUPPORTED_ANALYSIS_TYPES(NAME, CMD, CSV)                    \
  SupportedAnalysisTypes::NAME,
#include "SupportedAnalysisTypes.def"
};

constexpr llvm::StringRef to_string(SupportedAnalysisTypes AT) noexcept {
  switch (AT) {
#define PSR_PTABEN_SUPPORTED_ANALYSIS_TYPES(NAME, CMD, CSV)                    \
  case SupportedAnalysisTypes::NAME:                                           \
    return #NAME "Result";
#include "SupportedAnalysisTypes.def"
  }
}

} // namespace psr::ptaben
