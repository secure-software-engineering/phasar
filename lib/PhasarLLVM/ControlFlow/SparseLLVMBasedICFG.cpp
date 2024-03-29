#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFG.h"

#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"

#include "llvm/IR/IntrinsicInst.h"

#include <cassert>
#include <unordered_map>
#include <utility>

using namespace psr;

struct FVHasher {
  auto operator()(std::pair<const llvm::Function *, const llvm::Value *> FV)
      const noexcept {
    return llvm::hash_value(FV);
  }
};

struct SparseLLVMBasedICFG::CacheData {
  std::unordered_map<std::pair<f_t, v_t>, SparseLLVMBasedCFG, FVHasher> Cache{};
};

SparseLLVMBasedICFG::~SparseLLVMBasedICFG() = default;

SparseLLVMBasedICFG::SparseLLVMBasedICFG(
    LLVMProjectIRDB *IRDB, CallGraphAnalysisType CGType,
    llvm::ArrayRef<std::string> EntryPoints, LLVMTypeHierarchy *TH,
    LLVMAliasInfoRef PT, Soundness S, bool IncludeGlobals)
    : LLVMBasedICFG(IRDB, CGType, EntryPoints, TH, PT, S, IncludeGlobals),
      SparseCFGCache(new CacheData{}) {}

SparseLLVMBasedICFG::SparseLLVMBasedICFG(CallGraph<n_t, f_t> CG,
                                         LLVMProjectIRDB *IRDB,
                                         LLVMTypeHierarchy *TH)
    : LLVMBasedICFG(std::move(CG), IRDB, TH), SparseCFGCache(new CacheData{}) {}

SparseLLVMBasedICFG::SparseLLVMBasedICFG(LLVMProjectIRDB *IRDB,
                                         const nlohmann::json &SerializedCG,
                                         LLVMTypeHierarchy *TH)
    : LLVMBasedICFG(IRDB, SerializedCG, TH), SparseCFGCache(new CacheData{}) {}

static bool shouldKeepInst(const llvm::Instruction *Inst,
                           const llvm::Value *Val) {
  // TODO
  return true;
}

static void buildSparseCFG(const LLVMBasedCFG &CFG, SparseLLVMBasedCFG &SCFG,
                           const llvm::Function *Fun, const llvm::Value *Val) {
  llvm::SmallVector<
      std::pair<const llvm::Instruction *, const llvm::Instruction *>>
      WL;

  // -- Initialization

  const auto *Entry = &Fun->getEntryBlock().front();
  if (llvm::isa<llvm::DbgInfoIntrinsic>(Entry)) {
    Entry = Entry->getNextNonDebugInstruction();
  }

  for (const auto *Succ : CFG.getSuccsOf(Entry)) {
    WL.emplace_back(Entry, Succ);
  }

  // -- Fixpoint Iteration

  while (!WL.empty()) {
    auto [From, To] = WL.pop_back_val();

    // TODO
  }
}

const SparseLLVMBasedCFG &
SparseLLVMBasedICFG::getSparseCFGImpl(const llvm::Function *Fun,
                                      const llvm::Value *Val) const {
  assert(SparseCFGCache != nullptr);

  // TODO: Make thread-safe

  auto [It, Inserted] =
      SparseCFGCache->Cache.try_emplace(std::make_pair(Fun, Val));
  if (Inserted) {
    buildSparseCFG(*this, It->second, Fun, Val);
  }

  return It->second;
}
