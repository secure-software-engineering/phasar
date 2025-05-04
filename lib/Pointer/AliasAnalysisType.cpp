#include "phasar/Pointer/AliasAnalysisType.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"

std::string psr::toString(AliasAnalysisType PA) {
  switch (PA) {
  default:
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                               \
  case AliasAnalysisType::NAME:                                                \
    return #NAME;
#include "phasar/Pointer/AliasAnalysisType.def"
  case AliasAnalysisType::Invalid:
    return "Invalid";
  }
}

psr::AliasAnalysisType psr::toAliasAnalysisType(llvm::StringRef S) {
  AliasAnalysisType Type = llvm::StringSwitch<AliasAnalysisType>(S)
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE)                               \
  .Case(#NAME, AliasAnalysisType::NAME)
#include "phasar/Pointer/AliasAnalysisType.def"
                               .Default(AliasAnalysisType::Invalid);
  if (Type == AliasAnalysisType::Invalid) {
    Type = llvm::StringSwitch<AliasAnalysisType>(S)
#define ALIAS_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE)                               \
  .Case(CMDFLAG, AliasAnalysisType::NAME)
#include "phasar/Pointer/AliasAnalysisType.def"
               .Default(AliasAnalysisType::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   AliasAnalysisType PA) {
  return OS << toString(PA);
}
