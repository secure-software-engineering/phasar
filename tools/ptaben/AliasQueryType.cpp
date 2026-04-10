#include "AliasQueryType.h"

#include "phasar/Pointer/AliasResult.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"

#include <array>

using namespace psr;

llvm::StringRef ptaben::to_string(AliasQueryType QueryType) noexcept {
  switch (QueryType) {
#define ALIAS_QUERY_TYPE(NAME)                                                 \
  case AliasQueryType::NAME:                                                   \
    return #NAME;
#include "AliasQueryType.inc"
  }
  llvm_unreachable(
      "All AliasQueryType variants should be handled in the switch above");
}

static constexpr size_t NumQueryTypes = 0
#define ALIAS_QUERY_TYPE(NAME) +1
#include "AliasQueryType.inc"
    ;

struct NamedQuery {
  llvm::StringRef Name{};
  ptaben::AliasQueryType QueryType{};
};

static constexpr std::array<NamedQuery, NumQueryTypes>
getSortedQueryTypes() noexcept {
  using namespace psr;
  std::array<NamedQuery, NumQueryTypes> AllQueries = {{
#define ALIAS_QUERY_TYPE(NAME) {#NAME, ptaben::AliasQueryType::NAME},
#include "AliasQueryType.inc"
  }};

  // std::sort is not constexpr
  for (size_t I = 0; I < AllQueries.size(); ++I) {
    auto Max = I;
    for (size_t J = I + 1; J < AllQueries.size(); ++J) {
      if (AllQueries[J].Name.size() > AllQueries[Max].Name.size())
        Max = J;
    }
    auto Tmp = AllQueries[I];
    AllQueries[I] = AllQueries[Max];
    AllQueries[Max] = Tmp;
  }

  return AllQueries;
}

auto ptaben::parseAliasQueryType(llvm::StringRef Str) noexcept
    -> std::optional<AliasQueryType> {
  static constexpr auto AllQueries = getSortedQueryTypes();

  for (const auto &NQ : AllQueries) {
    if (Str.contains(NQ.Name)) {
      return NQ.QueryType;
    }
  }

  return std::nullopt;
}

auto ptaben::getExpectedAliasResult(AliasQueryType QT) noexcept -> AliasResult {
  switch (QT) {
  case AliasQueryType::MAYALIAS:
  case AliasQueryType::EXPECTEDFAILMAYALIAS:
  case AliasQueryType::PARTIALALIAS:
    return AliasResult::MayAlias;
  case AliasQueryType::MUSTALIAS:
  case AliasQueryType::EXPECTEDFAILMUSTALIAS:
    return AliasResult::MustAlias;
  case AliasQueryType::NOALIAS:
  case AliasQueryType::EXPECTEDFAILNOALIAS:
    return AliasResult::NoAlias;
  }

  llvm_unreachable(
      "All alias query types should be handled in the switch above");
}
