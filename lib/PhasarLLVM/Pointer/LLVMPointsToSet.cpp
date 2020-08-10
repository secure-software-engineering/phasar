/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <cassert>
#include <iostream>
#include <type_traits>
#include <unordered_set>

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

LLVMPointsToSet::LLVMPointsToSet(ProjectIRDB &IRDB, bool UseLazyEvaluation,
                                 PointerAnalysisType PATy)
    : PTA(IRDB, UseLazyEvaluation, PATy) {
  if (!UseLazyEvaluation) {
    for (llvm::Module *M : IRDB.getAllModules()) {
      // compute points-to information for all globals
      for (const auto &G : M->globals()) {
        computeValuesPointsToSet(&G);
      }
      // compute points-to information for all functions
      for (auto &F : *M) {
        if (!F.isDeclaration()) {
          computeFunctionsPointsToSet(&F);
        }
      }
    }
  }
}

void LLVMPointsToSet::computeValuesPointsToSet(const llvm::Value *V) {
  if (!isInterestingPointer(V)) {
    // don't need to do anything
    return;
  }
  // Add set for the queried value if none exists, yet
  addSingletonPointsToSet(V);
  if (const auto *G = llvm::dyn_cast<llvm::GlobalObject>(V)) {
    // A global object can be a function or a global variable. We need to
    // consider functions here, too, because function pointer magic may be
    // used by the target program. Add a set for global object.
    // A global object may be used in multiple functions.
    for (const auto *User : G->users()) {
      if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(User)) {
        computeFunctionsPointsToSet(
            const_cast<llvm::Function *>(Inst->getFunction()));
        if (isInterestingPointer(User)) {
          mergePointsToSets(User, G);
        } else if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
          if (isInterestingPointer(Store->getValueOperand())) {
            // Store->getPointerOperand() doesn't require checking: it is
            // always an interesting pointer
            mergePointsToSets(Store->getValueOperand(),
                              Store->getPointerOperand());
          }
        }
      }
    }
  } else {
    auto *VF = retrieveFunction(V);
    computeFunctionsPointsToSet(VF);
  }
}

void LLVMPointsToSet::addSingletonPointsToSet(const llvm::Value *V) {
  if (PointsToSets.find(V) != PointsToSets.end()) {
    PointsToSets[V]->insert(V);
  } else {
    PointsToSets[V] = std::make_shared<std::unordered_set<const llvm::Value *>>(
        std::unordered_set<const llvm::Value *>{V});
  }
}

void LLVMPointsToSet::mergePointsToSets(const llvm::Value *V1,
                                        const llvm::Value *V2) {
  auto SearchV1 = PointsToSets.find(V1);
  assert(SearchV1 != PointsToSets.end());
  auto SearchV2 = PointsToSets.find(V2);
  assert(SearchV2 != PointsToSets.end());
  const auto *V1Ptr = SearchV1->first;
  const auto *V2Ptr = SearchV2->first;
  if (V1Ptr == V2Ptr) {
    return;
  }
  auto V1Set = SearchV1->second;
  auto V2Set = SearchV2->second;
  // check if we need to merge the sets
  if (V1Set->find(V2) != V1Set->end()) {
    return;
  }
  std::shared_ptr<std::unordered_set<const llvm::Value *>> SmallerSet;
  std::shared_ptr<std::unordered_set<const llvm::Value *>> LargerSet;
  if (V1Set->size() <= V2Set->size()) {
    SmallerSet = V1Set;
    LargerSet = V2Set;
  } else {
    SmallerSet = V2Set;
    LargerSet = V1Set;
  }
  // add smaller set to larger one
  LargerSet->insert(SmallerSet->begin(), SmallerSet->end());
  // reindex the contents of the smaller set
  for (const auto *Ptr : *SmallerSet) {
    PointsToSets[Ptr] = LargerSet;
  }
  // get rid of the smaller set
  SmallerSet->clear();
}

void LLVMPointsToSet::computeFunctionsPointsToSet(llvm::Function *F) {
  // F may be null
  if (!F) {
    return;
  }
  // check if we already analyzed the function
  if (AnalyzedFunctions.find(F) != AnalyzedFunctions.end()) {
    return;
  }
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Analyzing function: " << F->getName().str());
  AnalyzedFunctions.insert(F);

  llvm::AAResults &AA = *PTA.getAAResults(F);
  bool EvalAAMD = true;

  // taken from llvm/Analysis/AliasAnalysisEvaluator.cpp
  const llvm::DataLayout &DL = F->getParent()->getDataLayout();

  llvm::SetVector<llvm::Value *> Pointers;
  llvm::SmallSetVector<llvm::CallBase *, 16> Calls;
  llvm::SetVector<llvm::Value *> Loads;
  llvm::SetVector<llvm::Value *> Stores;

  for (auto &I : F->args()) {
    if (I.getType()->isPointerTy()) { // Add all pointer arguments.
      Pointers.insert(&I);
    }
  }

  for (llvm::inst_iterator I = inst_begin(*F), E = inst_end(*F); I != E; ++I) {
    if (I->getType()->isPointerTy()) { // Add all pointer instructions.
      Pointers.insert(&*I);
    }
    if (EvalAAMD && llvm::isa<llvm::LoadInst>(&*I)) {
      Loads.insert(&*I);
    }
    if (EvalAAMD && llvm::isa<llvm::StoreInst>(&*I)) {
      Stores.insert(&*I);
    }
    llvm::Instruction &Inst = *I;
    if (auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
      llvm::Value *Callee = Call->getCalledValue();
      // Skip actual functions for direct function calls.
      if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee)) {
        Pointers.insert(Callee);
      }
      // Consider formals.
      for (llvm::Use &DataOp : Call->data_ops()) {
        if (isInterestingPointer(DataOp)) {
          Pointers.insert(DataOp);
        }
      }
      Calls.insert(Call);
    } else {
      // Consider all operands.
      for (llvm::Instruction::op_iterator OI = Inst.op_begin(),
                                          OE = Inst.op_end();
           OI != OE; ++OI) {
        if (isInterestingPointer(*OI)) {
          Pointers.insert(*OI);
        }
      }
    }
  }
  // introduce a singleton set for each pointer
  // those sets will be merged as we discover aliases
  for (auto *Pointer : Pointers) {
    addSingletonPointsToSet(Pointer);
  }
  // iterate over the worklist, and run the full (n^2)/2 disambiguations
  for (auto I1 = Pointers.begin(), E = Pointers.end(); I1 != E; ++I1) {
    llvm::Type *I1ElTy =
        llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
    const uint64_t I1Size = I1ElTy->isSized()
                                ? DL.getTypeStoreSize(I1ElTy)
                                : llvm::MemoryLocation::UnknownSize;
    for (auto I2 = Pointers.begin(); I2 != I1; ++I2) {
      llvm::Type *I2ElTy =
          llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
      const uint64_t I2Size = I2ElTy->isSized()
                                  ? DL.getTypeStoreSize(I2ElTy)
                                  : llvm::MemoryLocation::UnknownSize;
      switch (AA.alias(*I1, I1Size, *I2, I2Size)) {
      case llvm::NoAlias:
        // both pointers already have corresponding points-to sets, we are
        // fine
        break;
      case llvm::MayAlias: // NOLINT
        [[fallthrough]];
      case llvm::PartialAlias: // NOLINT
        [[fallthrough]];
      case llvm::MustAlias:
        // merge points to sets
        mergePointsToSets(*I1, *I2);
        break;
      }
    }
  }
  // we no longer need the LLVM representation
  PTA.erase(F);
}

AliasResult LLVMPointsToSet::alias(const llvm::Value *V1, const llvm::Value *V2,
                                   const llvm::Instruction *I) {
  // if V1 or V2 is not an interesting pointer those values cannot alias
  if (!isInterestingPointer(V1) || !isInterestingPointer(V2)) {
    return AliasResult::NoAlias;
  }
  computeValuesPointsToSet(V1);
  computeValuesPointsToSet(V2);
  return PointsToSets[V1]->count(V2) ? AliasResult::MustAlias
                                     : AliasResult::NoAlias;
}

std::shared_ptr<std::unordered_set<const llvm::Value *>>
LLVMPointsToSet::getPointsToSet(const llvm::Value *V,
                                const llvm::Instruction *I) {
  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return std::make_shared<std::unordered_set<const llvm::Value *>>();
  }
  // compute V's points-to set
  computeValuesPointsToSet(V);
  if (PointsToSets.find(V) == PointsToSets.end()) {
    // if we still can't find its value return an empty set
    return std::make_shared<std::unordered_set<const llvm::Value *>>();
  }
  return PointsToSets[V];
}

std::unordered_set<const llvm::Value *>
LLVMPointsToSet::getReachableAllocationSites(const llvm::Value *V,
                                             const llvm::Instruction *I) {
  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return std::unordered_set<const llvm::Value *>();
  }
  computeValuesPointsToSet(V);
  std::unordered_set<const llvm::Value *> AllocSites;
  const auto PTS = PointsToSets[V];
  for (const auto *P : *PTS) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(P)) {
      AllocSites.insert(Alloca);
    }
    if (llvm::isa<llvm::CallInst>(P) || llvm::isa<llvm::InvokeInst>(P)) {
      llvm::ImmutableCallSite CS(P);
      if (CS.getCalledFunction() != nullptr &&
          CS.getCalledFunction()->hasName() &&
          HeapAllocatingFunctions.count(CS.getCalledFunction()->getName())) {
        AllocSites.insert(P);
      }
    }
  }
  return AllocSites;
}

void LLVMPointsToSet::mergeWith(const PointsToInfo &PTI) {
  const auto *OtherPTI = dynamic_cast<const LLVMPointsToSet *>(&PTI);
  if (!OtherPTI) {
    llvm::report_fatal_error(
        "LLVMPointsToSet can only be merged with another LLVMPointsToSet!");
  }
  // merge analyzed functions
  AnalyzedFunctions.insert(OtherPTI->AnalyzedFunctions.begin(),
                           OtherPTI->AnalyzedFunctions.end());
  // merge points-to sets
  for (const auto &[KeyPtr, Set] : OtherPTI->PointsToSets) {
    bool FoundElemPtr = false;
    for (const auto *ElemPtr : *Set) {
      // check if a pointer of other is already present in this
      auto Search = PointsToSets.find(ElemPtr);
      if (Search != PointsToSets.end()) {
        // if so, copy its elements
        FoundElemPtr = true;
        Search->second->insert(Set->begin(), Set->end());
        // and reindex its elements
        for (const auto *ElemPtr : *Set) {
          PointsToSets.insert({ElemPtr, Search->second});
        }
        break;
      }
    }
    // if none of the pointers of a set of other is known in this, we need to
    // perform a copy
    if (!FoundElemPtr) {
      PointsToSets.insert(
          {KeyPtr,
           std::make_shared<std::unordered_set<const llvm::Value *>>(*Set)});
    }
  }
}

void LLVMPointsToSet::introduceAlias(const llvm::Value *V1,
                                     const llvm::Value *V2,
                                     const llvm::Instruction *I,
                                     AliasResult Kind) {
  //  only introduce aliases if both values are interesting pointer
  if (!isInterestingPointer(V1) || !isInterestingPointer(V2)) {
    return;
  }
  // before introducing additional aliases make sure we initially computed
  // the aliases for V1 and V2
  computeValuesPointsToSet(V1);
  computeValuesPointsToSet(V2);
  mergePointsToSets(V1, V2);
}

nlohmann::json LLVMPointsToSet::getAsJson() const { return ""_json; }

void LLVMPointsToSet::printAsJson(std::ostream &OS) const {}

void LLVMPointsToSet::print(std::ostream &OS) const {
  for (const auto &[V, PTS] : PointsToSets) {
    OS << "V: " << llvmIRToString(V) << '\n';
    for (const auto &Ptr : *PTS) {
      OS << "\tpoints to -> " << llvmIRToString(Ptr) << '\n';
    }
  }
}

} // namespace psr
