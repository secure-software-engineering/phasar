#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFG.h"

#include "phasar/ControlFlow/CallGraphData.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "SVFGCache.h"

#include <cassert>
#include <utility>

using namespace psr;

struct FVHasher {
  auto operator()(std::pair<const llvm::Function *, const llvm::Value *> FV)
      const noexcept {
    return llvm::hash_value(FV);
  }
};

SparseLLVMBasedICFG::~SparseLLVMBasedICFG() = default;

SparseLLVMBasedICFG::SparseLLVMBasedICFG(
    LLVMProjectIRDB *IRDB, CallGraphAnalysisType CGType,
    llvm::ArrayRef<std::string> EntryPoints, DIBasedTypeHierarchy *TH,
    LLVMAliasInfoRef PT, Soundness S, bool IncludeGlobals)
    : LLVMBasedICFG(IRDB, CGType, EntryPoints, TH, PT, S, IncludeGlobals),
      SparseCFGCache(new SVFGCache{}), AliasAnalysis(PT) {}

SparseLLVMBasedICFG::SparseLLVMBasedICFG(CallGraph<n_t, f_t> CG,
                                         LLVMProjectIRDB *IRDB,
                                         LLVMAliasInfoRef PT)
    : LLVMBasedICFG(std::move(CG), IRDB), SparseCFGCache(new SVFGCache{}),
      AliasAnalysis(PT) {}

SparseLLVMBasedICFG::SparseLLVMBasedICFG(LLVMProjectIRDB *IRDB,
                                         const CallGraphData &SerializedCG,
                                         LLVMAliasInfoRef PT)
    : LLVMBasedICFG(IRDB, SerializedCG), SparseCFGCache(new SVFGCache{}),
      AliasAnalysis(PT) {}

const SparseLLVMBasedCFG &
SparseLLVMBasedICFG::getSparseCFGImpl(const llvm::Function *Fun,
                                      const llvm::Value *Val) const {
  assert(SparseCFGCache != nullptr);
  return SparseCFGCache->getOrCreate(*this, Fun, Val, AliasAnalysis);
}
