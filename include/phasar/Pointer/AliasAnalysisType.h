#ifndef PHASAR_POINTER_ALIASANALYSISTYPE_H
#define PHASAR_POINTER_ALIASANALYSISTYPE_H

#include "phasar/Config/phasar-config.h" // For PHASAR_USE_SVF in AliasAnalysisType.def

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {

enum class AliasAnalysisType {
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE) NAME,
#include "phasar/Pointer/AliasAnalysisType.def"
  Invalid
};

[[nodiscard]] constexpr bool isAndersenOTFAA(AliasAnalysisType AATy) noexcept {
  switch (AATy) {
  case AliasAnalysisType::AndersenOTF:
  case AliasAnalysisType::AndersenOTFCtx:
  case AliasAnalysisType::AndersenOTFDynCtx:
    return true;
  default:
    return false;
  }
}

std::string toString(AliasAnalysisType PA);

AliasAnalysisType toAliasAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, AliasAnalysisType PA);
} // namespace psr

#endif // PHASAR_POINTER_ALIASANALYSISTYPE_H
