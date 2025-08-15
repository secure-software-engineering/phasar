/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CHAResolver.cpp
 *
 *  Created on: 20.07.2018
 *      Author: nicolas bellec
 */

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"

#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Function.h"

#include <memory>

using namespace psr;

CHAResolver::CHAResolver(const LLVMProjectIRDB *IRDB,
                         const LLVMVFTableProvider *VTP,
                         const DIBasedTypeHierarchy *TH)
    : Resolver(IRDB, VTP), TH(TH) {
  if (!TH) {
    this->TH = std::make_unique<DIBasedTypeHierarchy>(*IRDB);
  }
}

CHAResolver::~CHAResolver() = default;

void CHAResolver::resolveVirtualCall(FunctionSetTy &PossibleTargets,
                                     const llvm::CallBase *CallSite) {
  PHASAR_LOG_LEVEL(DEBUG, "Call virtual function: ");
  // Leading to SEGFAULT in Unittests. Error only when run in Debug mode
  // << llvmIRToString(CallSite));

  auto RetrievedVtableIndex = getVFTIndex(CallSite);
  if (!RetrievedVtableIndex.has_value()) {
    // An error occured
    PHASAR_LOG_LEVEL(DEBUG,
                     "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                         // Leading to SEGFAULT in Unittests. Error only when
                         // run in Debug mode
                         // << llvmIRToString(CallSite)
                         << "\n");
    return;
  }

  auto VtableIndex = RetrievedVtableIndex.value();

  PHASAR_LOG_LEVEL(DEBUG, "Virtual function table entry is: " << VtableIndex);

  const auto *ReceiverTy = getReceiverType(CallSite);

  // also insert all possible subtypes vtable entries
  auto FallbackTys = TH->getSubTypes(ReceiverTy);

  for (const auto &FallbackTy : FallbackTys) {
    const auto *Target = getNonPureVirtualVFTEntry(FallbackTy, VtableIndex,
                                                   CallSite, ReceiverTy);
    if (Target && psr::isConsistentCall(CallSite, Target)) {
      PossibleTargets.insert(Target);
    }
  }
}

std::string CHAResolver::str() const { return "CHA"; }
