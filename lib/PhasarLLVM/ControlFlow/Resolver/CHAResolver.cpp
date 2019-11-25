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

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

CHAResolver::CHAResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch)
    : Resolver(irdb, ch) {}

set<const llvm::Function *> CHAResolver::resolveVirtualCall(const llvm::ImmutableCallSite &CS) {
  set<const llvm::Function *> possible_call_targets;
  auto &lg = lg::get();

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Call virtual function: "
                << llvmIRToString(CS.getInstruction()));

  auto vtable_index = getVtableIndex(CS);
  if (vtable_index < 0) {
    // An error occured
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                  << llvmIRToString(CS.getInstruction()) << "\n");
    return {};
  }

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Virtual function table entry is: " << vtable_index);

  auto receiver_type = getReceiverType(CS);

  // also insert all possible subtypes vtable entries
  auto fallback_types =
      CH.getReachableSubTypes(receiver_type);

  for (auto &fallback : fallback_types) {
    insertVtableIntoResult(possible_call_targets, fallback, vtable_index,
                           CS);
  }

  return possible_call_targets;
}
