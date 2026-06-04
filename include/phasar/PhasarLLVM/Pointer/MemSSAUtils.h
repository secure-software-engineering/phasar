#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

namespace psr {

// Bundle of per-function analyses for the built-in MemorySSA provider.
// Members are declared in initialization order: each field depends only on
// the ones before it.
struct MemSSABundle {
  llvm::AssumptionCache AC;
  llvm::DominatorTree DT;
  llvm::BasicAAResult BAA;
  llvm::AAResults AA;
  llvm::MemorySSA MSSA;

  explicit MemSSABundle(llvm::Function &F, const llvm::TargetLibraryInfo *TLI);
};

/// Walks the MemorySSA def chain rooted at MA, collecting all StoreInst
/// reaching definitions into ReachingDefs.
/// Returns true if a LiveOnEntry def is reachable (value may come from outside
/// the function). In that case, ReachingDefs may be incompletely populated.
[[nodiscard]] bool collectReachingDefs(
    llvm::MemoryAccess *MA, const llvm::MemorySSA &MSSA,
    llvm::SmallPtrSetImpl<const llvm::StoreInst *> &ReachingDefs,
    llvm::SmallPtrSetImpl<llvm::MemoryAccess *> &Visited);

/// Collects all store instructions that may define the value loaded from the
/// given load. Forwards to the above collectReachingDefs overload.
[[nodiscard]] bool collectReachingDefs(
    const llvm::LoadInst *Load, llvm::MemorySSA &MSSA,
    llvm::SmallPtrSetImpl<const llvm::StoreInst *> &ReachingDefs);

} // namespace psr
