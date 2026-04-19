module;

#include "phasar/PhasarLLVM/Passes/ExampleModulePass.h"
#include "phasar/PhasarLLVM/Passes/GeneralStatisticsAnalysis.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"

export module phasar.llvm.passes;

export namespace psr {
using psr::ExampleModulePass;
using psr::GeneralStatistics;
using psr::GeneralStatisticsAnalysis;
using psr::ValueAnnotationPass;
} // namespace psr
