#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace psr {
enum class PointerAnalysisType {
#define POINTER_ANALYSIS_TYPE(NAME, CMDFLAG, TYPE) NAME,
#include "phasar/PhasarLLVM/Pointer/PointerAnalysisType.def"
  Invalid
};

std::string toString(PointerAnalysisType PA);

PointerAnalysisType toPointerAnalysisType(llvm::StringRef S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, PointerAnalysisType PA);
} // namespace psr
