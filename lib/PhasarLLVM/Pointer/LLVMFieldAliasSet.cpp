#include "phasar/PhasarLLVM/Pointer/LLVMFieldAliasSet.h"

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/MathExtras.h"

#include <cstddef>
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

static constexpr ptrdiff_t TopOffset = LLVMFieldAliasSet::AccessPath::TopOffset;

constexpr static void addOffset(ptrdiff_t &Into, ptrdiff_t Offs) noexcept {
  if (Into == TopOffset) {
    return;
  }

  if (llvm::AddOverflow(Into, Offs, Into)) {
    Into = TopOffset;
  }
}

auto LLVMFieldAliasSet::getAccessPath(const llvm::Value *Pointer) const
    -> AccessPath {
  // TODO: We may want to cache this!
  // -> See AbstractMemoryLocationFactory

  AccessPath Ret{Pointer, {0}};
  if (!Pointer || !Pointer->getType()->isPointerTy()) {
    return Ret;
  }

  while (true) {
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Pointer)) {
      Pointer = Load->getPointerOperand()->stripPointerCastsAndAliases();
      Ret.FieldAccesses.push_back(0);
      continue;
    }

    if (const auto *GEP = llvm::dyn_cast<llvm::GEPOperator>(Pointer)) {
      Pointer = GEP->getPointerOperand()->stripPointerCastsAndAliases();
      auto Offset = detail::AbstractMemoryLocationImpl::computeOffset(*DL, GEP);
      if (Offset) {
        addOffset(Ret.FieldAccesses.back(), *Offset);
      } else {
        Ret.FieldAccesses.back() = TopOffset;
      }

      continue;
    }
  }

  Ret.BasePtr = Pointer;
  return Ret;
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

auto LLVMFieldAliasSet::getAliasSet(v_t Pointer, n_t AtInstruction) const
    -> AliasSetPtrTy {
  auto Aliases = AS.getAliasSet(Pointer, AtInstruction);

  auto Ret = std::make_unique<AliasSetTy>();
  for (const auto *Alias : *Aliases) {
    Ret->insert(getAccessPath(Alias));
  }

  return Ret;
}
