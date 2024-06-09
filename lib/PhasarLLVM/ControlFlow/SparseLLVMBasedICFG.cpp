#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFG.h"

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
    llvm::ArrayRef<std::string> EntryPoints, LLVMTypeHierarchy *TH,
    LLVMAliasInfoRef PT, Soundness S, bool IncludeGlobals)
    : LLVMBasedICFG(IRDB, CGType, EntryPoints, TH, PT, S, IncludeGlobals),
      SparseCFGCache(new SVFGCache{}) {}

SparseLLVMBasedICFG::SparseLLVMBasedICFG(CallGraph<n_t, f_t> CG,
                                         LLVMProjectIRDB *IRDB)
    : LLVMBasedICFG(std::move(CG), IRDB), SparseCFGCache(new SVFGCache{}) {}

SparseLLVMBasedICFG::SparseLLVMBasedICFG(LLVMProjectIRDB *IRDB,
                                         const nlohmann::json &SerializedCG)
    : LLVMBasedICFG(IRDB, SerializedCG), SparseCFGCache(new SVFGCache{}) {}

const SparseLLVMBasedCFG &
SparseLLVMBasedICFG::getSparseCFGImpl(const llvm::Function *Fun,
                                      const llvm::Value *Val) const {
  assert(SparseCFGCache != nullptr);
  return SparseCFGCache->getOrCreate(*this, Fun, Val);
}
