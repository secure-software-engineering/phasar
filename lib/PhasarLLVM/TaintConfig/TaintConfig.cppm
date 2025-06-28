module;

#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigData.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigUtilities.h"

export module phasar.llvm.taintconfig;

export namespace psr {
using psr::collectGeneratedFacts;
using psr::collectLeakedFacts;
using psr::collectSanitizedFacts;
using psr::FunctionData;
using psr::LLVMTaintConfig;
using psr::parseTaintConfig;
using psr::parseTaintConfigOrNull;
using psr::TaintCategory;
using psr::TaintConfigBase;
using psr::TaintConfigData;
using psr::TaintConfigTraits;
using psr::VariableData;
} // namespace psr
