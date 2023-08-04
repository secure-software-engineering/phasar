/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/AbstractCallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <utility>

namespace psr::detail {

auto IDETypeStateAnalysisBase::getRelevantAllocas(d_t V) -> container_type {
  if (RelevantAllocaCache.find(V) != RelevantAllocaCache.end()) {
    return RelevantAllocaCache[V];
  }
  auto AliasSet = getWMAliasSet(V);
  container_type RelevantAllocas;
  PHASAR_LOG_LEVEL(DEBUG, "Compute relevant alloca's of " << DToString(V));
  for (const auto *Alias : AliasSet) {
    PHASAR_LOG_LEVEL(DEBUG, "Alias: " << DToString(Alias));
    // Collect the pointer operand of a aliased load instruciton
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
      if (hasMatchingType(Alias)) {
        PHASAR_LOG_LEVEL(
            DEBUG, " -> Alloca: " << DToString(Load->getPointerOperand()));
        RelevantAllocas.insert(Load->getPointerOperand());
      }
    } else {
      // For all other types of aliases, e.g. callsites, function arguments,
      // we check store instructions where thoses aliases are value operands.
      for (const auto *User : Alias->users()) {
        PHASAR_LOG_LEVEL(DEBUG, "  User: " << DToString(User));
        if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
          if (hasMatchingType(Store)) {
            PHASAR_LOG_LEVEL(DEBUG, "    -> Alloca: " << DToString(
                                        Store->getPointerOperand()));
            RelevantAllocas.insert(Store->getPointerOperand());
          }
        }
      }
    }
  }
  for (const auto *Alias : AliasSet) {
    RelevantAllocaCache[Alias] = RelevantAllocas;
  }
  return RelevantAllocas;
}

auto IDETypeStateAnalysisBase::getWMAliasSet(d_t V) -> container_type {
  if (AliasCache.find(V) != AliasCache.end()) {
    container_type AliasSet(AliasCache[V].begin(), AliasCache[V].end());
    return AliasSet;
  }
  auto PTS = PT.getAliasSet(V);
  for (const auto *Alias : *PTS) {
    if (hasMatchingType(Alias)) {
      AliasCache[Alias] = *PTS;
    }
  }
  container_type AliasSet(PTS->begin(), PTS->end());
  return AliasSet;
}

auto IDETypeStateAnalysisBase::getWMAliasesAndAllocas(d_t V) -> container_type {
  container_type AliasAndAllocas;
  container_type RelevantAllocas = getRelevantAllocas(V);
  container_type Aliases = getWMAliasSet(V);
  AliasAndAllocas.insert(Aliases.begin(), Aliases.end());
  AliasAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return AliasAndAllocas;
}

auto IDETypeStateAnalysisBase::getLocalAliasesAndAllocas(
    d_t V, llvm::StringRef /*Fname*/) -> container_type {
  container_type AliasAndAllocas;
  container_type RelevantAllocas = getRelevantAllocas(V);
  container_type Aliases; // =
                          // IRDB->getAliasGraph(Fname)->getAliasSet(V);
  for (const auto *Alias : Aliases) {
    if (hasMatchingType(Alias)) {
      AliasAndAllocas.insert(Alias);
    }
  }
  // AliasAndAllocas.insert(Aliases.begin(), Aliases.end());
  AliasAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return AliasAndAllocas;
}

static bool hasMatchingTypeName(
    const llvm::Type *Ty,
    const std::function<bool(llvm::StringRef)> &IsTypeNameOfInterest) {
  if (const auto *StructTy = llvm::dyn_cast<llvm::StructType>(Ty)) {
    return IsTypeNameOfInterest(StructTy->getName());
  }
  // primitive type
  std::string Str;
  llvm::raw_string_ostream S(Str);
  S << *Ty;
  S.flush();
  return IsTypeNameOfInterest(Str);
}

bool IDETypeStateAnalysisBase::hasMatchingType(d_t V) {
  // General case
  if (V->getType()->isPointerTy()) {
    if (hasMatchingTypeName(V->getType()->getPointerElementType(),
                            IsTypeNameOfInterest)) {
      return true;
    }
  }
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Alloca->getAllocatedType()->getPointerElementType(),
              IsTypeNameOfInterest)) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
    if (Load->getPointerOperand()
            ->getType()
            ->getPointerElementType()
            ->isPointerTy()) {
      if (hasMatchingTypeName(Load->getPointerOperand()
                                  ->getType()
                                  ->getPointerElementType()
                                  ->getPointerElementType(),
                              IsTypeNameOfInterest)) {
        return true;
      }
    }
    return false;
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (hasMatchingTypeName(
              Store->getValueOperand()->getType()->getPointerElementType(),
              IsTypeNameOfInterest)) {
        return true;
      }
    }
    return false;
  }
  return false;
}

} // namespace psr::detail
