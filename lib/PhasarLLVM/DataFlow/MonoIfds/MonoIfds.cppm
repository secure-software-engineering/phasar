module;

#include "phasar/PhasarLLVM/DataFlow/MonoIfds/AliasCache.h"
#include "phasar/PhasarLLVM/DataFlow/MonoIfds/Problems/MonoIFDSTaintAnalysis.h"

export module phasar.llvm.dataflow.monoifds;

export namespace psr {
using psr::monoifds::AliasCache;
using psr::monoifds::TaintAnalysis;
} // namespace psr
