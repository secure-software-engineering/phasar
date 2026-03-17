#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"

#include <optional>

namespace psr {
enum class AliasQueryType {
#define ALIAS_QUERY_TYPE(NAME) NAME,
#include "AliasQueryType.inc"
};

[[nodiscard]] llvm::StringRef to_string(AliasQueryType QueryType) noexcept;
[[nodiscard]] std::optional<AliasQueryType>
parseAliasQueryType(llvm::StringRef Str) noexcept;

enum class AliasResult;

[[nodiscard]] AliasResult getExpectedAliasResult(AliasQueryType QT) noexcept;

} // namespace psr
