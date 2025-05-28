module;

#include "phasar/ControlFlow.h"
#include "phasar/ControlFlow/SparseCFGBase.h"
#include "phasar/ControlFlow/SparseCFGProvider.h"

export module phasar.controlflow;

export namespace psr {
using psr::CallGraph;
using psr::CallGraphAnalysisType;
using psr::CallGraphBuilder;
using psr::CGTraits;
using psr::toCallGraphAnalysisType;
using psr::toString;
using psr::operator<<;
using psr::CallGraphBase;
using psr::CallGraphData;
using psr::CFGBase;
using psr::CFGTraits;
using psr::has_getSparseCFG;
using psr::has_getSparseCFG_v;
using psr::ICFGBase;
using psr::is_cfg_v;
using psr::is_icfg_v;
using psr::is_sparse_cfg_v;
using psr::SparseCFGBase;
using psr::SparseCFGProvider;
using psr::SpecialMemberFunctionType;
using psr::toSpecialMemberFunctionType;
using psr::valueOf;
} // namespace psr
