#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedICFG.h"

#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"

#include "llvm/IR/Argument.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"

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

static const llvm::Type *getPointeeTypeOrNull(const llvm::Value *V) {
  // TODO
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    return Alloca->getAllocatedType();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    if (const auto *ByValTy = Arg->getParamByValType()) {
      return ByValTy;
    }
    if (const auto *ByValTy = Arg->getParamStructRetType()) {
      return ByValTy;
    }
  }

  // TODO: Handle more cases

  return nullptr;
}

static bool fuzzyMayAlias(const llvm::Value * /*Ptr1*/,
                          const llvm::Type *PointeeTy1,
                          const llvm::Value * /*Ptr2*/,
                          const llvm::Type *PointeeTy2) {
  // Pointers to pointers may alias with any pointer, because the analysis may
  // not be field-sensitive.
  // If we don't know the pointee-type (PointeeTyN == nullptr), we cannot assume
  // anything.

  if (!PointeeTy1 || PointeeTy1->isPointerTy()) {
    return true;
  }

  if (!PointeeTy2 || PointeeTy2->isPointerTy()) {
    return true;
  }

  return PointeeTy1 == PointeeTy2;
}

static bool shouldKeepInst(const llvm::Instruction *Inst,
                           const llvm::Value *Val) {
  if (Inst == Val || llvm::pred_size(Inst->getParent()) != 1) {
    // First in BB always stays for now
    return true;
  }

  const auto *ValTy = Val->getType();
  bool ValPtr = ValTy->isPointerTy();
  const auto *PointeeTy = ValPtr ? getPointeeTypeOrNull(Val) : nullptr;

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst)) {
    if (llvm::isa<llvm::GlobalValue>(Val)) {
      return true;
    }
  }

  for (const auto *Op : Inst->operand_values()) {
    if (Op == Val) {
      return true;
    }
    const auto *OpTy = Op->getType();
    bool OpPtr = OpTy->isPointerTy();

    if (ValPtr != OpPtr) {
      // Pointers cannot influence non-pointers
      continue;
    }
    if (!ValPtr) {
      continue;
    }

    if (fuzzyMayAlias(Val, PointeeTy, Op, getPointeeTypeOrNull(Op))) {
      return true;
    }
  }

  // TODO
  return false;
}

static void buildSparseCFG(const LLVMBasedCFG &CFG,
                           SparseLLVMBasedCFG::vgraph_t &SCFG,
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

    const auto *Curr = From;
    if (shouldKeepInst(To, Val)) {
      Curr = To;
      auto Inserted = SCFG.try_emplace(From, To).second;
      if (!Inserted) {
        continue;
      }
    }

    for (const auto *Succ : CFG.getSuccsOf(To)) {
      WL.emplace_back(Curr, Succ);
    }
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
    buildSparseCFG(*this, It->second.VGraph, Fun, Val);
  }

  return It->second;
}
