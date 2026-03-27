#include "phasar/ControlFlow/ControlFlowOrder.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/ArraySet.h"
#include "phasar/Utils/FunctionCompressor.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

using namespace psr;

void psr::computeCFGOrder(ControlFlowOrder &Into, const llvm::Function *Fun) {

  llvm::SmallDenseSet<const llvm::Instruction *> Seen;

  const auto Visit = [&, CFG = psr::LLVMBasedCFG()](
                         auto &Visit, const llvm::Instruction *Inst) {
    if (!Seen.insert(Inst).second) {
      return;
    }

    const auto *Next = Inst->getNextNonDebugInstruction();
    if (Next) {
      Visit(Visit, Next);
    } else {
      for (const auto *Succ : CFG.getSuccsOf(Inst)) {
        Visit(Visit, Succ);
      }
    }

    Into.Order.insert(Inst);
  };

  if (!Fun->isDeclaration()) {
    Visit(Visit, &Fun->getEntryBlock().front());
  }
}

constexpr static auto psrGetExitPoints(const auto *Fun) {
  if constexpr (requires() { psr::getAllExitPoints(Fun, true); }) {
    return psr::getAllExitPoints(Fun, /*IncludeResume=*/true);
  } else {
    return psr::getAllExitPoints(Fun);
  }
}

static llvm::SmallDenseMap<const llvm::Instruction *, size_t>
computeDistanceToRet(llvm::ArrayRef<FunctionId> Funs,
                     const SCCHolder<FunctionId> &SCCs, SCCId<FunctionId> SCC,
                     const FunctionCompressor &Functions,
                     const psr::LLVMBasedCallGraph &CG) {
  llvm::SmallDenseMap<const llvm::Instruction *, size_t> Ret;

  ArraySet<const llvm::Instruction *> WL;

  for (auto FunId : Funs) {
    const auto *Fun = Functions[FunId];
    for (const auto *ExitInst : psrGetExitPoints(Fun)) {
      WL.insert(ExitInst);
      Ret[ExitInst] = 0;
    }

    WL.foreach ([&, CFG = psr::LLVMBasedCFG()](const auto *Inst) {
      const auto Dist = Ret[Inst];
      const auto NextDist = [&] {
        size_t NextDist = Dist + 1;
        if (llvm::isa<llvm::CallBase>(Inst)) {
          for (const auto *Callee : CG.getCalleesOfCallAt(Inst)) {
            const auto CalleeId = Functions.getOrNull(Callee);
            if (CalleeId && SCCs.SCCOfNode[*CalleeId] == SCC) {
              // some heuristics...
              NextDist += 100;
            }
          }
        }
        return NextDist;
      }();

      for (const auto *Pred : CFG.getPredsOf(Inst)) {
        auto [It, Inserted] = Ret.try_emplace(Pred, NextDist);
        if (!Inserted && It->second > NextDist) {
          It->second = NextDist;
          Inserted = true;
        }

        if (Inserted) {
          WL.insert(Pred);
        }
      }
    });
  }

  return Ret;
}

void psr::computeCFGOrder(ControlFlowOrder &Into,
                          const SCCHolder<FunctionId> &SCCs,
                          SCCId<FunctionId> SCC,
                          const psr::LLVMBasedCallGraph &CG,
                          const FunctionCompressor &Functions) {
  llvm::SmallDenseSet<const llvm::Instruction *> Seen;

  const auto &Funs = SCCs.NodesInSCC[SCC];

  const auto DistToRet = computeDistanceToRet(Funs, SCCs, SCC, Functions, CG);

  const auto Visit = [&, CFG = psr::LLVMBasedCFG()](
                         auto &Visit, const llvm::Instruction *Inst) {
    if (!Seen.insert(Inst).second) {
      return;
    }

    if (llvm::isa<llvm::ReturnInst, llvm::ResumeInst>(Inst)) {
      for (const auto *CS : CG.getCallersOf(Inst->getFunction())) {
        const auto *Caller = CS->getFunction();
        const auto CallerId = Functions.getOrNull(Caller);
        if (CallerId && SCCs.SCCOfNode[*CallerId] == SCC) {
          Visit(Visit, CS);
        }
      }
    }

    const auto *Next = Inst->getNextNonDebugInstruction();
    if (Next) {
      Visit(Visit, Next);
    } else {
      auto Succs = CFG.getSuccsOf(Inst);
      llvm::sort(Succs, [&](const auto *I1, const auto *I2) {
        return DistToRet.lookup(I1) < DistToRet.lookup(I2);
      });
      for (const auto *Succ : Succs) {
        Visit(Visit, Succ);
      }
    }

    Into.Order.insert(Inst);
  };

  assert(!Funs.empty());

  const llvm::Function *LargestFun = nullptr;
  size_t LargestSz = 0;
  for (auto FunId : Funs) {
    const auto *Fun = Functions[FunId];
    if (Fun->isDeclaration()) {
      continue;
    }

    const auto Callers = CG.getCallersOf(Fun);
    const auto *OutsideCaller = llvm::find_if(Callers, [&](const auto *Caller) {
      const auto CallerId = Functions.getOrNull(Caller->getFunction());
      return CallerId && SCCs.SCCOfNode[*CallerId] != SCC;
    });
    if (OutsideCaller == Callers.end()) {
      continue;
    }

    const auto FunSz = Fun->size();
    if (FunSz > LargestSz) {
      LargestFun = Fun;
      LargestSz = FunSz;
    }
  }

  assert(!LargestFun || !LargestFun->isDeclaration());
  if (LargestFun) {
    Visit(Visit, &LargestFun->getEntryBlock().front());
  }

  for (auto FunId : Funs) {
    const auto *Fun = Functions[FunId];
    if (!Fun->isDeclaration()) {
      Visit(Visit, &Fun->getEntryBlock().front());
    }
  }
}
