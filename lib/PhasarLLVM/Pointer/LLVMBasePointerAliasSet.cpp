#include "phasar/PhasarLLVM/Pointer/LLVMBasePointerAliasSet.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <memory>

using namespace psr;

const llvm::Value *
LLVMBasePointerAliasSet::getBasePointer(const llvm::Value *Pointer) {
  if (!Pointer || !Pointer->getType()->isPointerTy()) {
    return Pointer;
  }

  while (true) {
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Pointer)) {
      Pointer = Load->getPointerOperand()->stripPointerCastsAndAliases();
      continue;
    }

    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Pointer)) {
      Pointer = GEP->getPointerOperand()->stripPointerCastsAndAliases();
      continue;
    }

    break;
  }

  return Pointer;
}

auto LLVMBasePointerAliasSet::getAliasSet(v_t Pointer, n_t AtInstruction) const
    -> AliasSetPtrTy {
  auto Aliases = AS.getAliasSet(Pointer, AtInstruction);

  auto Ret = std::make_unique<AliasSetTy>();
  for (const auto *Alias : *Aliases) {
    Ret->insert(getBasePointer(Alias));
  }

  return Ret;
}
