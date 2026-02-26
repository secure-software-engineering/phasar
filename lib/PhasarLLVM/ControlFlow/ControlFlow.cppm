module;

#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/NOResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/VTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFGProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFGView.h"

export module phasar.llvm.controlflow;

export namespace psr {
using psr::buildLLVMBasedCallGraph;
using psr::CFGTraits;
using psr::CHAResolver;
using psr::getDefaultEntryPoints;
using psr::getEntryFunctions;
using psr::getEntryFunctionsMut;
using psr::getNonPureVirtualVFTEntry;
using psr::getReceiverType;
using psr::getReceiverTypeName;
using psr::getVFTIndex;
using psr::GlobalCtorsDtorsModel;
using psr::ICFGBase;
using psr::isConsistentCall;
using psr::isHeapAllocatingFunction;
using psr::isVirtualCall;
using psr::LLVMBasedBackwardCFG;
using psr::LLVMBasedBackwardICFG;
using psr::LLVMBasedCallGraph;
using psr::LLVMBasedCFG;
using psr::LLVMBasedICFG;
using psr::LLVMVFTableProvider;
using psr::NOResolver;
using psr::OTFResolver;
using psr::Resolver;
using psr::RTAResolver;
using psr::SparseLLVMBasedCFG;
using psr::SparseLLVMBasedCFGProvider;
using psr::SparseLLVMBasedICFG;
using psr::SparseLLVMBasedICFGView;
using psr::valueOf;
using psr::VTAResolver;
} // namespace psr
