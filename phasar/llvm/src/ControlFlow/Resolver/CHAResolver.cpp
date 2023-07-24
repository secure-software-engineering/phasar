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

#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

using namespace std;
using namespace psr;

CHAResolver::CHAResolver(LLVMProjectIRDB &IRDB, LLVMTypeHierarchy &TH)
    : Resolver(IRDB, TH) {}

auto CHAResolver::resolveVirtualCall(const llvm::CallBase *CallSite)
    -> FunctionSetTy {
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
    return {};
  }

  auto VtableIndex = RetrievedVtableIndex.value();

  PHASAR_LOG_LEVEL(DEBUG, "Virtual function table entry is: " << VtableIndex);

  const auto *ReceiverTy = getReceiverType(CallSite);

  // also insert all possible subtypes vtable entries
  auto FallbackTys = Resolver::TH->getSubTypes(ReceiverTy);

  FunctionSetTy PossibleCallees;

  for (const auto &FallbackTy : FallbackTys) {
    const auto *Target =
        getNonPureVirtualVFTEntry(FallbackTy, VtableIndex, CallSite);
    if (Target) {
      PossibleCallees.insert(Target);
    }
  }
  return PossibleCallees;
}

std::string CHAResolver::str() const { return "CHA"; }
