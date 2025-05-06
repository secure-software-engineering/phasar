module;

#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"

export module phasar.llvm;
import phasar.llvm_db;
import phasar.llvm_typehierarchy;
import phasar.llvm_controlflow;
import phasar.llvm_pointer;

export namespace psr {
using psr::createAnalysisProblem;
using psr::HelperAnalyses;
} // namespace psr
