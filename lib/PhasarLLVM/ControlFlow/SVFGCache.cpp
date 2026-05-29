#include "SVFGCache.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMControlFlow.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ModRef.h"

using namespace psr;

static void buildSparseCFG(const LLVMBasedCFG &CFG,
                           SparseLLVMBasedCFG::vgraph_t &SCFG,
                           const llvm::Function *Fun, const llvm::Value *Val,
                           LLVMAliasInfoRef AliasAnalysis) {
  llvm::SmallVector<
      std::pair<const llvm::Instruction *, const llvm::Instruction *>>
      WL;

  // -- Initialization

  const auto *Entry = &Fun->getEntryBlock().front();
#if LLVM_VERSION_MAJOR <= 18
  if (llvm::isa<llvm::DbgInfoIntrinsic>(Entry)) {
    Entry = Entry->getNextNonDebugInstruction();
  }
#endif

  for (const auto *Succ : CFG.getSuccsOf(Entry)) {
    WL.emplace_back(Entry, Succ);
  }

  // -- Fixpoint Iteration

  llvm::SmallDenseSet<const llvm::Instruction *> Handled;

  while (!WL.empty()) {
    auto [From, To] = WL.pop_back_val();

    const auto *Curr = From;
    if (SparseLLVMControlFlow::shouldKeepInst(To, Val, AliasAnalysis)) {
      Curr = To;
      auto [It, Inserted] = SCFG.try_emplace(From, To);
      if (!Inserted) {
        if (It->second != To) {
          It->second = nullptr;
        }
      }
    }

    if (!Handled.insert(To).second) {
      continue;
    }

    for (const auto *Succ : CFG.getSuccsOf(To)) {
      WL.emplace_back(Curr, Succ);
    }
  }
}

const SparseLLVMBasedCFG &
SVFGCache::getOrCreate(const LLVMBasedCFG &CFG, const llvm::Function *Fun,
                       const llvm::Value *Val, LLVMAliasInfoRef AliasAnalysis) {
  // XXX: Make thread-safe

  auto [It, Inserted] = Cache.try_emplace(std::make_pair(Fun, Val));
  if (Inserted) {
    buildSparseCFG(CFG, It->second.VGraph, Fun, Val, AliasAnalysis);
  }

  return It->second;
}
