module;

#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSetData.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedAliasAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"

export module phasar.llvm.pointer;

export namespace psr {
using psr::AliasInfoTraits;
using psr::FilteredLLVMAliasSet;
using psr::isInterestingPointer;
using psr::LLVMAliasInfo;
using psr::LLVMAliasInfoRef;
using psr::LLVMAliasSet;
using psr::LLVMAliasSetData;
using psr::LLVMBasedAliasAnalysis;
using LLVMAliasInfoRef =
    AliasInfoRef<const llvm::Value *, const llvm::Instruction *>;
using LLVMAliasInfo = AliasInfo<const llvm::Value *, const llvm::Instruction *>;
} // namespace psr
