#include "SVFGCache.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFG.h"

#include "llvm/IR/IntrinsicInst.h"

using namespace psr;

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

static bool isNonPointerType(const llvm::Type *Ty) {
  if (const auto *Struct = llvm::dyn_cast<llvm::StructType>(Ty)) {
    for (const auto *ElemTy : Struct->elements()) {
      // TODO: Go into nested structs recursively
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

static bool fuzzyMayAlias(const llvm::Value *Ptr1, const llvm::Type *PointeeTy1,
                          const llvm::Value *Ptr2,
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

  if (isNonAddressTakenVariable(Ptr1) || isNonAddressTakenVariable(Ptr2)) {
    return false;
  }

  return PointeeTy1 == PointeeTy2;
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
                           const llvm::Value *Val) {
  if (Inst == Val || isFirstInBB(Inst) || isLastInBB(Inst, Val)) {
    // First in BB always stays for now

    // llvm::errs() << "[shouldKeepInst]: 1: " << llvmIRToString(Inst)
    //              << " :: " << llvmIRToShortString(Val) << '\n';
    return true;
  }

  const auto *ValTy = Val->getType();
  bool ValPtr = ValTy->isPointerTy();
  const auto *PointeeTy = ValPtr ? getPointeeTypeOrNull(Val) : nullptr;

  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst)) {
    if (llvm::isa<llvm::GlobalValue>(Val)) {
      // llvm::errs() << "[shouldKeepInst]: 2: " << llvmIRToString(Inst)
      //              << " :: " << llvmIRToShortString(Val) << '\n';
      return true;
    }
  }

  for (const auto *Op : Inst->operand_values()) {
    if (Op == Val) {
      // llvm::errs() << "[shouldKeepInst]: 3.1: " << llvmIRToString(Inst)
      //              << " :: " << llvmIRToShortString(Val) << '\n';
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

    if (fuzzyMayAlias(Val, PointeeTy, Op, getPointeeTypeOrNull(Op))) {
      // llvm::errs() << "[shouldKeepInst]: 3: " << llvmIRToString(Inst)
      //              << " :: " << llvmIRToShortString(Val) << '\n';
      return true;
    }
  }

  // llvm::errs() << "[shouldKeepInst]: FALSE: " << llvmIRToString(Inst)
  //              << " :: " << llvmIRToShortString(Val) << '\n';
  // TODO
  return false;
}

static void buildSparseCFG(const LLVMBasedCFG &CFG,
                           SparseLLVMBasedCFG::vgraph_t &SCFG,
                           const llvm::Function *Fun, const llvm::Value *Val) {

  // llvm::errs() << "Build SCFG for '" << Fun->getName() << "' and value "
  //              << llvmIRToString(Val) << '\n';
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
    if (shouldKeepInst(To, Val)) {
      Curr = To;
      auto [It, Inserted] = SCFG.try_emplace(From, To);
      if (!Inserted) {
        if (It->second != To) {
          // llvm::errs() << "[buildSparseCFG]: Ambiguity at "
          //              << llvmIRToString(From) << " ::> "
          //              << llvmIRToShortString(It->second) << " VS "
          //              << llvmIRToShortString(To) << '\n';
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

const SparseLLVMBasedCFG &SVFGCache::getOrCreate(const LLVMBasedCFG &CFG,
                                                 const llvm::Function *Fun,
                                                 const llvm::Value *Val) {
  // TODO: Make thread-safe

  auto [It, Inserted] = Cache.try_emplace(std::make_pair(Fun, Val));
  if (Inserted) {
    buildSparseCFG(CFG, It->second.VGraph, Fun, Val);
  }

  return It->second;
}
