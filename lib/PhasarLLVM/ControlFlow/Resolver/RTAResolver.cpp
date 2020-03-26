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

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

using namespace std;
using namespace psr;

RTAResolver::RTAResolver(ProjectIRDB &IRDB, LLVMTypeHierarchy &TH)
    : CHAResolver(IRDB, TH) {}

// void RTAResolver::firstFunction(const llvm::Function *F) {
//   auto func_type = F->getFunctionType();

//   for (auto param : func_type->params()) {
//     if (llvm::isa<llvm::PointerType>(param)) {
//       if (auto struct_ty =
//               llvm::dyn_cast<llvm::StructType>(stripPointer(param))) {
//         unsound_types.insert(struct_ty);
//       }
//     }
//   }
// }

set<const llvm::Function *>
RTAResolver::resolveVirtualCall(llvm::ImmutableCallSite CS) {
  // throw runtime_error("RTA is currently unabled to deal with already built "
  //                     "library, it has been disable until this is fixed");

  set<const llvm::Function *> possible_call_targets;
  auto &lg = lg::get();

  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "Call virtual function: "
                << llvmIRToString(CS.getInstruction()));

  auto vtable_index = getVFTIndex(CS);
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
  auto reachable_types = Resolver::TH->getSubTypes(receiver_type);

  // also insert all possible subtypes vtable entries
  auto possible_types = IRDB.getAllocatedStructTypes();

  auto end_it = reachable_types.end();
  for (auto possible_type : possible_types) {
    if (auto possible_type_struct =
            llvm::dyn_cast<llvm::StructType>(possible_type)) {
      if (reachable_types.find(possible_type_struct) != end_it) {
        auto Target =
            getNonPureVirtualVFTEntry(possible_type_struct, vtable_index, CS);
        if (Target) {
          possible_call_targets.insert(Target);
        }
      }
    }
  }

  if (possible_call_targets.size() == 0)
    return CHAResolver::resolveVirtualCall(CS);

  return possible_call_targets;
}
