module;

#include "phasar/PhasarLLVM/Pointer.h"

export module phasar.llvm.pointer;

export namespace psr {
using psr::AliasAnalysisView;
using psr::AliasInfoTraits;
using psr::AndersenOTFResult;
using psr::AndersenOTFSolver;
using psr::collectReachingDefs;
using psr::computeAndersenOTF;
using psr::computeAndersenOTFRaw;
using psr::computeBotCtxIndSensUnionFindAA;
using psr::computeBotCtxIndSensUnionFindAARaw;
using psr::computeBotCtxSensUnionFindAA;
using psr::computeBotCtxSensUnionFindAARaw;
using psr::computeCtxIndSensUnionFindAA;
using psr::computeCtxIndSensUnionFindAARaw;
using psr::computeCtxSensUnionFindAA;
using psr::computeCtxSensUnionFindAARaw;
using psr::computeIndSensUnionFindAA;
using psr::computeIndSensUnionFindAARaw;
using psr::computeUnionFindAA;
using psr::computeUnionFindAARaw;
using psr::FilteredLLVMAliasSet;
using psr::FunctionAliasView;
using psr::GlobalInitCache;
using psr::isInterestingPointer;
using psr::LLVMAliasInfo;
using psr::LLVMAliasInfoRef;
using psr::LLVMAliasIteratorRef;
using psr::LLVMAliasSet;
using psr::LLVMAliasSetData;
using psr::LLVMLocalRawAliasIterator;
using psr::LLVMLocalRawAliasIteratorMixin;
using psr::LLVMLocalUnionFindAliasIterator;
using psr::LLVMPointsToIterator;
using psr::LLVMPointsToIteratorRef;
using psr::llvmRawAliasHandler;
using psr::LLVMRawAliasIterator;
using psr::LLVMRawAliasIteratorMixin;
using psr::LLVMUnionFindAliasIterator;
using psr::MemSSABundle;
using psr::pag::LLVMCGProvider;

#ifdef PHASAR_USE_SVF
using psr::createLLVMSVFPointsToIterator;
using psr::createSVFDDAPointsToInfo;
using psr::createSVFVFSPointsToInfo;
using psr::SVFBasedPointsToInfo;
using psr::SVFBasedPointsToInfoRef;
using psr::SVFPointsToInfoTraits;
#endif

} // namespace psr
