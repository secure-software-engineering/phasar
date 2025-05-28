module;

#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/SimpleAnalysisConstructor.h"

export module phasar.llvm;

export import phasar.llvm.controlflow;
export import phasar.llvm.dataflow;
export import phasar.llvm.db;
export import phasar.llvm.domain;
export import phasar.llvm.passes;
export import phasar.llvm.pointer;
export import phasar.llvm.taintconfig;
export import phasar.llvm.typehierarchy;
export import phasar.llvm.utils;

export namespace psr {
using psr::createAnalysisProblem;
using psr::HelperAnalyses;
} // namespace psr
