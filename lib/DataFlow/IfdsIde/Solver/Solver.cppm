module;

#include "phasar/DataFlow/IfdsIde/Solver/Compressor.h"
#include "phasar/DataFlow/IfdsIde/Solver/EdgeFunctionCache.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCacheNG.h"
#include "phasar/DataFlow/IfdsIde/Solver/IFDSSolver.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"

export module phasar.dataflow.ifdside.solver;

export namespace psr {
using psr::Compressor;
using psr::DefaultMapKeyCompressor;
using psr::EdgeFunctionCache;
using psr::EdgeFunctionCacheStats;
using psr::EdgeFunctionKind;
using psr::ESGEdgeKind;
using psr::FlowEdgeFunctionCache;
using psr::FlowEdgeFunctionCacheNG;
using psr::IDELinearConstantAnalysis;
using psr::IFDSSolver;
using psr::IFDSSolverTest;
using psr::LLVMMapKeyCompressor;
using psr::MapKeyCompressorCombinator;
using psr::NodeCompressorTraits;
using psr::NoneCompressor;
using psr::solveIDEProblem;
using psr::ValCompressorTraits;
} // namespace psr
