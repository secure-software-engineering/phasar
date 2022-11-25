#include "phasar/PhasarLLVM/Pointer/PointerAnalysisType.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"

std::string psr::toString(PointerAnalysisType PA) {
  switch (PA) {
  default:
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                             \
  case PointerAnalysisType::NAME:                                              \
    return #NAME;
#include "phasar/PhasarLLVM/Pointer/PointerAnalysisType.def"
  case PointerAnalysisType::Invalid:
    return "Invalid";
  }
}

psr::PointerAnalysisType psr::toPointerAnalysisType(llvm::StringRef S) {
  PointerAnalysisType Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE)                             \
  .Case(#NAME, PointerAnalysisType::NAME)
#include "phasar/PhasarLLVM/Pointer/PointerAnalysisType.def"
                                 .Default(PointerAnalysisType::Invalid);
  if (Type == PointerAnalysisType::Invalid) {
    Type = llvm::StringSwitch<PointerAnalysisType>(S)
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE)                             \
  .Case(CMDFLAG, PointerAnalysisType::NAME)
#include "phasar/PhasarLLVM/Pointer/PointerAnalysisType.def"
               .Default(PointerAnalysisType::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   PointerAnalysisType PA) {
  return OS << toString(PA);
}
