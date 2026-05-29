#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMControlFlow.h"

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"

using namespace psr;

#if LLVM_VERSION_MAJOR <= 20
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
#endif

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
#if LLVM_VERSION_MAJOR <= 20
      if (Call->paramHasAttr(ArgNo, llvm::Attribute::NoCapture) &&
          isNonPointerType(Call->getType())) {
        continue;
      }
      return false;
#else
      auto Captures = Call->getCaptureInfo(ArgNo);
      auto CComp = Captures.getOtherComponents() | Captures.getRetComponents();
      if (llvm::capturesAnyProvenance(CComp) ||
          (llvm::capturesAddress(CComp) &&
           !llvm::capturesAddressIsNullOnly(CComp))) {
        return false;
      }
      continue;
#endif
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

bool SparseLLVMControlFlow::shouldKeepInst(n_t Inst, v_t Val,
                                           LLVMAliasInfoRef AI) {
  if (Inst == Val || isFirstInBB(Inst) || isLastInBB(Inst, Val)) {
    // First in BB always stays for now
    return true;
  }
  if (llvm::isa<llvm::CallBase>(Inst)) {
    if (llvm::isa<llvm::GlobalValue>(Val)) {
      // We cannot know, whether the callee uses the global
      return true;
    }
  }

  const auto *ValTy = Val->getType();
  bool ValPtr = ValTy->isPointerTy();

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

    if (mayAlias(Val, Op, AI)) {
      return true;
    }
  }

  return false;
}

auto psr::SparseLLVMControlFlow::advanceToNextUserImplInternal(
    n_t Succ, v_t Fact, LLVMAliasInfoRef AI) -> n_t {
  while (!shouldKeepInst(Succ, Fact, AI)) {
    n_t NextSucc =
#if LLVM_VERSION_MAJOR <= 18
        Succ->getNextNonDebugInstruction();
#else
        Succ->getNextNode();
#endif
    if (!NextSucc) {
      break;
    }
    Succ = NextSucc;
  }
  return Succ;
}
