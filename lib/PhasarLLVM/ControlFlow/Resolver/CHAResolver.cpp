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
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

using namespace std;
using namespace psr;

CHAResolver::CHAResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch)
    : Resolver(irdb, ch) {}

set<string> CHAResolver::resolveVirtualCall(const llvm::ImmutableCallSite &CS) {
  set<string> possible_call_targets;
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

  auto receiver_type_name = getReceiverTypeName(CS);

  // also insert all possible subtypes vtable entries
  auto fallback_type_names =
      CH.getTransitivelyReachableTypes(receiver_type_name);

  for (auto &fallback_name : fallback_type_names) {
    insertVtableIntoResult(possible_call_targets, fallback_name, vtable_index,
                           CS);
  }

  return possible_call_targets;
}
