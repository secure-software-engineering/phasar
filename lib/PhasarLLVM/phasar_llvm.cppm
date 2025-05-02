module;

#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"

export module phasar.phasar_llvm;
import phasar.phasar_llvm_db;
import phasar.phasar_llvm_typehierarchy;
import phasar.phasar_llvm_controlflow;
import phasar.phasar_llvm_pointer;

export namespace psr {
using psr::createAnalysisProblem;
using psr::HelperAnalyses;
} // namespace psr
