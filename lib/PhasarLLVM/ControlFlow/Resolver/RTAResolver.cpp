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

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
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

auto RTAResolver::resolveVirtualCall(const llvm::CallBase *CallSite)
    -> FunctionSetTy {
  // throw runtime_error("RTA is currently unabled to deal with already built "
  //                     "library, it has been disable until this is fixed");

  FunctionSetTy PossibleCallTargets;

  PHASAR_LOG_LEVEL(DEBUG,
                   "Call virtual function: " << llvmIRToString(CallSite));

  auto RetrievedVtableIndex = getVFTIndex(CallSite);
  if (!RetrievedVtableIndex.has_value()) {
    // An error occured
    PHASAR_LOG_LEVEL(DEBUG,
                     "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                         << llvmIRToString(CallSite) << "\n");
    return {};
  }

  auto VtableIndex = RetrievedVtableIndex.value();

  PHASAR_LOG_LEVEL(DEBUG, "Virtual function table entry is: " << VtableIndex);

  const auto *ReceiverType = getReceiverType(CallSite);

  // also insert all possible subtypes vtable entries
  auto ReachableTypes = Resolver::TH->getSubTypes(ReceiverType);

  // also insert all possible subtypes vtable entries
  auto PossibleTypes = IRDB.getAllocatedStructTypes();

  auto EndIt = ReachableTypes.end();
  for (const auto *PossibleType : PossibleTypes) {
    if (const auto *PossibleTypeStruct =
            llvm::dyn_cast<llvm::StructType>(PossibleType)) {
      if (ReachableTypes.find(PossibleTypeStruct) != EndIt) {
        const auto *Target = getNonPureVirtualVFTEntry(PossibleTypeStruct,
                                                       VtableIndex, CallSite);
        if (Target) {
          PossibleCallTargets.insert(Target);
        }
      }
    }
  }

  if (PossibleCallTargets.empty()) {
    return CHAResolver::resolveVirtualCall(CallSite);
  }

  return PossibleCallTargets;
}

std::string RTAResolver::str() const { return "RTA"; }
