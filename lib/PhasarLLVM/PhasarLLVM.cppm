module;

#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"

export module phasar.llvm;
import phasar.llvm.db;
import phasar.llvm.typehierarchy;
import phasar.llvm.controlflow;
import phasar.llvm.pointer;

export namespace psr {
using psr::createAnalysisProblem;
using psr::HelperAnalyses;
} // namespace psr
