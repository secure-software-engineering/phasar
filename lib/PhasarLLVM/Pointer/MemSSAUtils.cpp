/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/MemSSAUtils.h"

#include "phasar/Utils/Utilities.h"

using namespace psr;

MemSSABundle::MemSSABundle(llvm::Function &F,
                           const llvm::TargetLibraryInfo *TLI)
    : AC(F), DT(F), TBAA(
#if LLVM_VERSION_MAJOR > 19
                        /*UsingTypeSanitizer=*/false
#endif
                        ),
      SNA(),
      BAA(F.getParent()->getDataLayout(), F, assertNotNull(TLI), AC, &DT),
      AA([](const auto *TLI, auto *TBAA, auto *SNA, auto *BAA) {
        llvm::AAResults AA(*TLI);
        AA.addAAResult(*TBAA);
        AA.addAAResult(*SNA);
        AA.addAAResult(*BAA);
        return AA;
      }(TLI, &TBAA, &SNA, &BAA)),
      MSSA(F, &AA, &DT) {
}

bool psr::collectReachingDefs(
    llvm::MemoryAccess *MA, const llvm::MemorySSA &MSSA,
    llvm::SmallPtrSetImpl<const llvm::StoreInst *> &ReachingDefs,
    llvm::SmallPtrSetImpl<llvm::MemoryAccess *> &Visited) {
  if (!Visited.insert(MA).second) {
    return false;
  }
  if (MSSA.isLiveOnEntryDef(MA)) {
    return true;
  }
  if (auto *Def = llvm::dyn_cast<llvm::MemoryDef>(MA)) {
    // We only care about stores for now
    if (const auto *St =
            llvm::dyn_cast<llvm::StoreInst>(Def->getMemoryInst())) {
      ReachingDefs.insert(St);
      return false;
    }
    return true;
  }
  if (auto *Phi = llvm::dyn_cast<llvm::MemoryPhi>(MA)) {
    for (const auto &Inc : Phi->incoming_values()) {
      bool LOE = collectReachingDefs(llvm::cast<llvm::MemoryAccess>(Inc.get()),
                                     MSSA, ReachingDefs, Visited);
      if (LOE) {
        return true;
      }
    }
  }
  return false;
}

bool psr::collectReachingDefs(
    const llvm::LoadInst *Load, llvm::MemorySSA &MSSA,
    llvm::SmallPtrSetImpl<const llvm::StoreInst *> &ReachingDefs) {
  if (auto *Access = MSSA.getMemoryAccess(Load)) {
    auto *Clobber = MSSA.getWalker()->getClobberingMemoryAccess(Access);
    llvm::SmallPtrSet<llvm::MemoryAccess *, 8> Visited;
    return collectReachingDefs(Clobber, MSSA, ReachingDefs, Visited);
  }

  return true;
}
