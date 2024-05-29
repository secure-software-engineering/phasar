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

#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/StringSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

using namespace std;
using namespace psr;

RTAResolver::RTAResolver(const LLVMProjectIRDB *IRDB,
                         const LLVMVFTableProvider *VTP,
                         const LLVMTypeHierarchy *TH)
    : CHAResolver(IRDB, VTP, TH) {
  resolveAllocatedStructTypes();
}

auto RTAResolver::resolveVirtualCall(const llvm::CallBase *CallSite)
    -> FunctionSetTy {

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
  auto ReachableTypes = TH->getSubTypes(ReceiverType);

  // also insert all possible subtypes vtable entries

  auto EndIt = ReachableTypes.end();
  for (const auto *PossibleType : AllocatedStructTypes) {
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

/// More or less copied from GeneralStatisticsAnalysis
void RTAResolver::resolveAllocatedStructTypes() {
  if (!AllocatedStructTypes.empty()) {
    return;
  }

  llvm::DenseSet<const llvm::StructType *> AllocatedStructTypes;

  for (const auto *Fun : IRDB->getAllFunctions()) {
    for (const auto &Inst : llvm::instructions(Fun)) {
      if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&Inst)) {
        if (const auto *StructTy =
                llvm::dyn_cast<llvm::StructType>(Alloca->getAllocatedType())) {
          AllocatedStructTypes.insert(StructTy);
        }
      } else if (const auto *CallSite = llvm::dyn_cast<llvm::CallBase>(&Inst);
                 CallSite && CallSite->getCalledFunction()) {
        // check if an instance of a user-defined type is allocated on the
        // heap

        if (!isHeapAllocatingFunction(CallSite->getCalledFunction())) {
          continue;
        }
        /// TODO: Does this iteration over the users make sense?
        /// After LLVM 15 we will probably not be able to access the
        /// PointerElementType anyway...
        for (const auto *User : Inst.users()) {
          const auto *Cast = llvm::dyn_cast<llvm::BitCastInst>(User);
          if (!Cast || Cast->getDestTy()->isOpaquePointerTy() ||
              !Cast->getDestTy()
                   ->getNonOpaquePointerElementType()
                   ->isStructTy()) {
            continue;
          }
          // finally check for ctor call
          for (const auto *User : Cast->users()) {
            if (const auto *CTor = llvm::dyn_cast<llvm::CallBase>(User)) {
              // potential call to the structures ctor
              if (CTor->getCalledFunction() &&
                  getNthFunctionArgument(CTor->getCalledFunction(), 0)
                          ->getType() == Cast->getDestTy() &&
                  !Cast->getDestTy()->isOpaquePointerTy()) {
                if (const auto *StructTy = llvm::dyn_cast<llvm::StructType>(
                        Cast->getDestTy()->getNonOpaquePointerElementType())) {
                  AllocatedStructTypes.insert(StructTy);
                }
              }
            }
          }
        }
      }
    }
  }

  this->AllocatedStructTypes.reserve(AllocatedStructTypes.size());
  this->AllocatedStructTypes.insert(this->AllocatedStructTypes.end(),
                                    AllocatedStructTypes.begin(),
                                    AllocatedStructTypes.end());
}
