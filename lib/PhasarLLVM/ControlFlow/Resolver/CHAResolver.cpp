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

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

CHAResolver::CHAResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH)
    : Resolver(IRDB, TH) {}

set<const llvm::Function *>
CHAResolver::resolveVirtualCall(llvm::ImmutableCallSite CS) {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Call virtual function: "
                << llvmIRToString(CS.getInstruction()));

  auto VFTIdx = getVFTIndex(CS);
  if (VFTIdx < 0) {
    // An error occured
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                  << llvmIRToString(CS.getInstruction()) << "\n");
    return {};
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Virtual function table entry is: " << VFTIdx);

  const auto *ReceiverTy = getReceiverType(CS);

  // also insert all possible subtypes vtable entries
  auto FallbackTys = Resolver::TH->getSubTypes(ReceiverTy);

  set<const llvm::Function *> PossibleCallees;

  for (const auto &FallbackTy : FallbackTys) {
    const auto *Target = getNonPureVirtualVFTEntry(FallbackTy, VFTIdx, CS);
    if (Target) {
      PossibleCallees.insert(Target);
    }
  }
  return PossibleCallees;
}
