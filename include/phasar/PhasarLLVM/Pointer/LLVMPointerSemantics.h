#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <optional>

/// LLVM-level questions that LLVMPAGBuilder and AndersenOTFSolver must answer
/// identically. They emit different node and edge kinds, but must not disagree
/// on which IR constructs carry pointers.

namespace psr {

/// Whether Ptr is a memory-location (alloca or global), cast to an integer.
///
/// Useful for handling atomicrmw of pointers, which clang punns to i64.
[[nodiscard]] inline bool isPunnedPointerAccess(const llvm::DataLayout &DL,
                                                const llvm::Value *Ptr,
                                                const llvm::Type *AccessedTy) {
  if (!AccessedTy->isIntegerTy(DL.getPointerSizeInBits())) {
    return false;
  }
  const llvm::Value *Base = Ptr->stripPointerCastsAndAliases();
  if (const auto *A = llvm::dyn_cast<llvm::AllocaInst>(Base)) {
    return !definitelyContainsNoPointer(A->getAllocatedType());
  }
  if (const auto *G = llvm::dyn_cast<llvm::GlobalVariable>(Base)) {
    return !definitelyContainsNoPointer(G->getValueType());
  }
  return false;
}

/// The memory access to model for a load, store, atomicrmw or cmpxchg.
struct LLVMMemoryAccess {
  const llvm::Instruction *Instr{};
  const llvm::Value *Pointer{};
  /// Null if the access only reads.
  const llvm::Value *StoredValue{};
  /// Null if the access only writes; \c Instr itself for an atomic.
  const llvm::Instruction *LoadedInto{};
  bool Punned{};

  /// The value whose type decides whether a pointer is transferred. For a
  /// cmpxchg that is the new value, not the { ty, i1 } result.
  [[nodiscard]] const llvm::Value *transferredValue() const noexcept {
    return StoredValue ? StoredValue : LoadedInto;
  }

  /// Whether the access has to be modeled at all. Gating on
  /// definitelyContainsNoPointer alone would drop punned accesses.
  [[nodiscard]] bool mayTransferPointer() const {
    return Punned || !definitelyContainsNoPointer(transferredValue());
  }
};

/// Decomposes \p I, or returns nullopt if it is not a memory access. Field-
/// insensitively an atomicrmw is a store of the new value plus a load of the
/// old one; cmpxchg likewise, into its { ty, i1 } result.
///
/// The result still has to pass mayTransferPointer().
[[nodiscard]] inline std::optional<LLVMMemoryAccess>
asMemoryAccess(const llvm::Instruction &I, const llvm::DataLayout &DL) {
  const auto Make = [&DL, &I](const llvm::Value *Ptr,
                              const llvm::Value *StoredValue,
                              const llvm::Instruction *LoadedInto) {
    const auto *Transferred = StoredValue ? StoredValue : LoadedInto;
    return LLVMMemoryAccess{
        .Instr = &I,
        .Pointer = Ptr,
        .StoredValue = StoredValue,
        .LoadedInto = LoadedInto,
        .Punned = isPunnedPointerAccess(DL, Ptr, Transferred->getType()),
    };
  };

  if (const auto *S = llvm::dyn_cast<llvm::StoreInst>(&I)) {
    return Make(S->getPointerOperand(), S->getValueOperand(), nullptr);
  }
  if (const auto *L = llvm::dyn_cast<llvm::LoadInst>(&I)) {
    return Make(L->getPointerOperand(), nullptr, L);
  }
  if (const auto *RMW = llvm::dyn_cast<llvm::AtomicRMWInst>(&I)) {
    return Make(RMW->getPointerOperand(), RMW->getValOperand(), RMW);
  }
  if (const auto *CX = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&I)) {
    return Make(CX->getPointerOperand(), CX->getNewValOperand(), CX);
  }
  return std::nullopt;
}

} // namespace psr
