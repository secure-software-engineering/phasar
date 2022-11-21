#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"

std::string psr::toString(CallGraphAnalysisType CGA) {
  switch (CGA) {
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  case CallGraphAnalysisType::NAME:                                            \
    return #NAME;
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.def"
  case CallGraphAnalysisType::Invalid:
    return "Invalid";
  }
}

psr::CallGraphAnalysisType psr::toCallGraphAnalysisType(llvm::StringRef S) {
  CallGraphAnalysisType Type = llvm::StringSwitch<CallGraphAnalysisType>(S)
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  .Case(#NAME, CallGraphAnalysisType::NAME)
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.def"
                                   .Default(CallGraphAnalysisType::Invalid);
  if (Type == CallGraphAnalysisType::Invalid) {
    Type = llvm::StringSwitch<CallGraphAnalysisType>(S)
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC)                          \
  .Case(CMDFLAG, CallGraphAnalysisType::NAME)
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.def"
               .Default(CallGraphAnalysisType::Invalid);
  }
  return Type;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   CallGraphAnalysisType CGA) {
  return OS << toString(CGA);
}
