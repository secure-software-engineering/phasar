#ifndef PHASAR_PHASARLLVM_POINTER_ALIASANALYSISTYPE_H_
#define PHASAR_PHASARLLVM_POINTER_ALIASANALYSISTYPE_H_

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
enum class AliasAnalysisType {
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE) NAME,
#include "phasar/Pointer/AliasAnalysisType.def"
  Invalid
};

std::string toString(AliasAnalysisType PA);

AliasAnalysisType toAliasAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, AliasAnalysisType PA);
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_ALIASANALYSISTYPE_H_
