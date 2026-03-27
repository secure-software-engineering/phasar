#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Pointer/AliasResult.h"

#include "llvm/ADT/StringRef.h"

#include <optional>

namespace psr::ptaben {
enum class AliasQueryType {
#define ALIAS_QUERY_TYPE(NAME) NAME,
#include "AliasQueryType.inc"
};

[[nodiscard]] llvm::StringRef to_string(AliasQueryType QueryType) noexcept;
[[nodiscard]] std::optional<AliasQueryType>
parseAliasQueryType(llvm::StringRef Str) noexcept;

[[nodiscard]] AliasResult getExpectedAliasResult(AliasQueryType QT) noexcept;

} // namespace psr::ptaben
