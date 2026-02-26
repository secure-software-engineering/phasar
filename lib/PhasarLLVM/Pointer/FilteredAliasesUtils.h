#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Compiler.h"

LLVM_LIBRARY_VISIBILITY inline const llvm::Function *
getFunction(const llvm::Value *V) {
  if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    return Inst->getFunction();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent();
  }
  return nullptr;
}

[[nodiscard]] LLVM_LIBRARY_VISIBILITY inline bool
isConstantGlobalValue(const llvm::GlobalValue *GlobV) {
  if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(GlobV)) {
    return Glob->isConstant();
  }
  if (const auto *Alias = llvm::dyn_cast<llvm::GlobalAlias>(GlobV)) {
    if (const auto *AliasGlob =
            llvm::dyn_cast<llvm::GlobalVariable>(Alias->getAliasee())) {
      return AliasGlob->isConstant();
    }
  }
  return true;
}

LLVM_LIBRARY_VISIBILITY inline bool mustNoalias(const llvm::Value *P1,
                                                const llvm::Value *P2) {
  if (P1 == P2) {
    return false;
  }
  assert(P1);
  assert(P2);

  // Assumptions:
  // - Globals do not alias with allocas
  // - Globals do not alias with each other (this may be a bit unsound, though)
  // - Allocas do not alias each other (relax a bit for allocas of pointers)
  // - Constant globals are not generated as data-flow facts

  if (const auto *Alloca1 = llvm::dyn_cast<llvm::AllocaInst>(P1)) {
    if (llvm::isa<llvm::GlobalValue>(P2)) {
      return true;
    }
    if (const auto *Alloca2 = llvm::dyn_cast<llvm::AllocaInst>(P2)) {
      return !Alloca1->getAllocatedType()->isPointerTy() &&
             !Alloca2->getAllocatedType()->isPointerTy();
    }
  } else if (const auto *Glob1 = llvm::dyn_cast<llvm::GlobalValue>(P1)) {
    if (llvm::isa<llvm::AllocaInst>(P2) || isConstantGlobalValue(Glob1)) {
      return true;
    }
    if (llvm::isa<llvm::GlobalValue>(P2)) {
      return true; // approximation
    }
  } else if (const auto *Glob2 = llvm::dyn_cast<llvm::GlobalValue>(P2)) {
    return isConstantGlobalValue(Glob2);
  }

  return false;
}
