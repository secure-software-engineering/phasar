/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * RTAResolver.cpp
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
#include <phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>

using namespace std;
using namespace psr;

RTAResolver::RTAResolver(ProjectIRDB &irdb, LLVMTypeHierarchy &ch)
    : CHAResolver(irdb, ch) {}

void RTAResolver::firstFunction(const llvm::Function *F) {
  auto func_type = F->getFunctionType();

  for (auto param : func_type->params()) {
    if (llvm::isa<llvm::PointerType>(param)) {
      if (auto struct_ty =
              llvm::dyn_cast<llvm::StructType>(stripPointer(param))) {
        unsound_types.insert(struct_ty);
      }
    }
  }
}

set<string> RTAResolver::resolveVirtualCall(const llvm::ImmutableCallSite &CS) {
  throw runtime_error("RTA is currently unabled to deal with already built "
                      "library, it has been disable until this is fixed");

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

  auto receiver_type = getReceiverType(CS);
  auto receiver_type_name = receiver_type->getName().str();

  if (unsound_types.find(receiver_type) != unsound_types.end()) {
    return CHAResolver::resolveVirtualCall(CS);
  }

  // also insert all possible subtypes vtable entries
  auto reachable_type_names =
      CH.getTransitivelyReachableTypes(receiver_type_name);

  // also insert all possible subtypes vtable entries
  auto possible_types = IRDB.getAllocatedTypes();

  auto end_it = reachable_type_names.end();
  for (auto possible_type : possible_types) {
    if (auto possible_type_struct =
            llvm::dyn_cast<llvm::StructType>(possible_type)) {
      string type_name = possible_type_struct->getName().str();
      if (reachable_type_names.find(type_name) != end_it) {
        insertVtableIntoResult(possible_call_targets, type_name, vtable_index,
                               CS);
      }
    }
  }

  if (possible_call_targets.size() == 0)
    return CHAResolver::resolveVirtualCall(CS);

  return possible_call_targets;
}
