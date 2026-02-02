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

namespace psr {
enum class UnionFindAliasAnalysisType : uint8_t {
#define UNIONFIND_ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, DESC) NAME,
#include "phasar/Pointer/UnionFindAAType.def"
};

[[nodiscard]] constexpr llvm::StringRef
to_string(UnionFindAliasAnalysisType AType) noexcept {
  switch (AType) {
#define UNIONFIND_ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                     \
  case UnionFindAliasAnalysisType::NAME:                                       \
    return CMDFLAG;
#include "phasar/Pointer/UnionFindAAType.def"
  }
}
} // namespace psr
