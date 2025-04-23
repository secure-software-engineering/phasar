#include "SVFGCache.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"

using namespace psr;

static bool isNonPointerType(const llvm::Type *Ty) {
  if (const auto *Struct = llvm::dyn_cast<llvm::StructType>(Ty)) {
    for (const auto *ElemTy : Struct->elements()) {
      // XXX: Go into nested structs recursively
      if (!ElemTy->isSingleValueType() || ElemTy->isVectorTy()) {
        return false;
      }
    }
    return true;
  }
  if (const auto *Vec = llvm::dyn_cast<llvm::VectorType>(Ty)) {
    return !Vec->getElementType()->isPointerTy();
  }
  return Ty->isSingleValueType();
}

static bool isNonAddressTakenVariable(const llvm::Value *Val) {
  const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Val);
  if (!Alloca) {
    return false;
  }
  for (const auto &Use : Alloca->uses()) {
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Use.getUser())) {
      if (Use == Store->getValueOperand()) {
        return false;
      }
    } else if (const auto *Call =
                   llvm::dyn_cast<llvm::CallBase>(Use.getUser())) {
      auto ArgNo = Use.getOperandNo();
      if (Call->paramHasAttr(ArgNo, llvm::Attribute::StructRet)) {
        continue;
      }
      if (Call->paramHasAttr(ArgNo, llvm::Attribute::NoCapture) &&
          isNonPointerType(Call->getType())) {
        continue;
      }
      return false;
    }
  }
  return true;
}

static bool mayAlias(const llvm::Value *Ptr1, const llvm::Value *Ptr2,
                     LLVMAliasInfoRef AliasAnalysis) {
  if (isNonAddressTakenVariable(Ptr1) || isNonAddressTakenVariable(Ptr2)) {
    return false;
  }

  return AliasAnalysis.alias(Ptr1, Ptr2) != AliasResult::NoAlias;
}

static bool isFirstInBB(const llvm::Instruction *Inst) {
  return !Inst->getPrevNode();
}

static bool isLastInBB(const llvm::Instruction *Inst, const llvm::Value *Val) {
  if (Inst->getNextNode()) {
    return false;
  }

  if (Val->getType()->isPointerTy()) {
    return true;
  }

  const auto *InstBB = Inst->getParent();
  for (const auto *User : Val->users()) {
    const auto *UserInst = llvm::dyn_cast<llvm::Instruction>(User);
    if (!UserInst || UserInst->getParent() != InstBB) {
      return true;
    }
  }
  return llvm::succ_empty(Inst);
}

static bool shouldKeepInst(const llvm::Instruction *Inst,
                           const llvm::Value *Val,
                           LLVMAliasInfoRef AliasAnalysis) {
  if (Inst == Val || isFirstInBB(Inst) || isLastInBB(Inst, Val)) {
    // First in BB always stays for now
    return true;
  }

  const auto *ValTy = Val->getType();
  bool ValPtr = ValTy->isPointerTy();

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst)) {
    if (llvm::isa<llvm::GlobalValue>(Val)) {
      return true;
    }
  }

  for (const auto *Op : Inst->operand_values()) {
    if (Op == Val) {
      return true;
    }
    if (!ValPtr) {
      continue;
    }
    const auto *OpTy = Op->getType();
    bool OpPtr = OpTy->isPointerTy();

    if (!OpPtr) {
      // Pointers cannot influence non-pointers
      continue;
    }

    if (mayAlias(Val, Op, AliasAnalysis)) {
      return true;
    }
  }

  return false;
}

static void buildSparseCFG(const LLVMBasedCFG &CFG,
                           SparseLLVMBasedCFG::vgraph_t &SCFG,
                           const llvm::Function *Fun, const llvm::Value *Val,
                           LLVMAliasInfoRef AliasAnalysis) {
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

  llvm::SmallDenseSet<const llvm::Instruction *> Handled;

  while (!WL.empty()) {
    auto [From, To] = WL.pop_back_val();

    const auto *Curr = From;
    if (shouldKeepInst(To, Val, AliasAnalysis)) {
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
