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
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace psr;

RTAResolver::RTAResolver(const LLVMProjectIRDB *IRDB,
                         const LLVMVFTableProvider *VTP,
                         const DIBasedTypeHierarchy *TH)
    : CHAResolver(IRDB, VTP, TH) {
  resolveAllocatedCompositeTypes();
}

void RTAResolver::resolveVirtualCall(FunctionSetTy &PossibleTargets,
                                     const llvm::CallBase *CallSite) {

  PHASAR_LOG_LEVEL(DEBUG,
                   "Call virtual function: " << llvmIRToString(CallSite));

  auto RetrievedVtableIndex = getVFTIndex(CallSite);
  if (!RetrievedVtableIndex.has_value()) {
    // An error occured
    PHASAR_LOG_LEVEL(DEBUG,
                     "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                         << llvmIRToString(CallSite) << "\n");
    return;
  }

  auto VtableIndex = RetrievedVtableIndex.value();

  PHASAR_LOG_LEVEL(DEBUG, "Virtual function table entry is: " << VtableIndex);

  const auto *ReceiverType = getReceiverType(CallSite);

  // also insert all possible subtypes vtable entries
  auto ReachableTypes = TH->getSubTypes(ReceiverType);

  // also insert all possible subtypes vtable entries
  auto EndIt = ReachableTypes.end();
  for (const auto *PossibleType : AllocatedCompositeTypes) {
    if (ReachableTypes.find(PossibleType) != EndIt) {

      const auto *Target = getNonPureVirtualVFTEntry(PossibleType, VtableIndex,
                                                     CallSite, ReceiverType);
      if (Target && psr::isConsistentCall(CallSite, Target)) {
        PossibleTargets.insert(Target);
      }
    }
  }

  if (PossibleTargets.empty()) {
    CHAResolver::resolveVirtualCall(PossibleTargets, CallSite);
  }
}

std::string RTAResolver::str() const { return "RTA"; }

static const llvm::DICompositeType *
isCompositeStructType(const llvm::DIType *Ty) {
  if (const auto *CompTy = llvm::dyn_cast_if_present<llvm::DICompositeType>(Ty);
      CompTy && (CompTy->getTag() == llvm::dwarf::DW_TAG_structure_type ||
                 CompTy->getTag() == llvm::dwarf::DW_TAG_class_type)) {

    return CompTy;
  }

  return nullptr;
}

void RTAResolver::resolveAllocatedCompositeTypes() {
  if (!AllocatedCompositeTypes.empty()) {
    return;
  }

  llvm::DenseSet<const llvm::DICompositeType *> AllocatedTypes;

  for (const auto *Inst : IRDB->getAllInstructions()) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Inst)) {
      if (const auto *Ty = isCompositeStructType(getVarTypeFromIR(Alloca))) {
        AllocatedTypes.insert(Ty);
      }
    } else if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Inst)) {
      if (const auto *Callee = llvm::dyn_cast<llvm::Function>(
              Call->getCalledOperand()->stripPointerCastsAndAliases())) {
        if (psr::isHeapAllocatingFunction(Callee)) {
          const auto *MDNode = Call->getMetadata("heapallocsite");
          if (const auto *CompTy = llvm::
#if LLVM_VERSION_MAJOR >= 15
                  dyn_cast_if_present
#else
                  dyn_cast_or_null
#endif
              <llvm::DICompositeType>(MDNode);
              isCompositeStructType(CompTy)) {

            AllocatedTypes.insert(CompTy);
          }
        }
      }
    }
  }

  AllocatedCompositeTypes.reserve(AllocatedTypes.size());
  AllocatedCompositeTypes.insert(AllocatedCompositeTypes.end(),
                                 AllocatedTypes.begin(), AllocatedTypes.end());
}
