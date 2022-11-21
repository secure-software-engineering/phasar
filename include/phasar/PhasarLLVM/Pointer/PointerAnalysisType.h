#ifndef PHASAR_PHASARLLVM_POINTER_POINTERANALYSISTYPE_H_
#define PHASAR_PHASARLLVM_POINTER_POINTERANALYSISTYPE_H_

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

#endif // PHASAR_PHASARLLVM_POINTER_POINTERANALYSISTYPE_H_
