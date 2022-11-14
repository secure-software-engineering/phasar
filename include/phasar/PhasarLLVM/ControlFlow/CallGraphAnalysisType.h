#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
enum class CallGraphAnalysisType {
#define CALL_GRAPH_ANALYSIS_TYPE(NAME, CMDFLAG, DESC) NAME,
#include "phasar/PhasarLLVM/ControlFlow/CallGraphAnalysisType.def"
  Invalid
};

std::string toString(CallGraphAnalysisType CGA);

CallGraphAnalysisType toCallGraphAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, CallGraphAnalysisType CGA);
} // namespace psr
