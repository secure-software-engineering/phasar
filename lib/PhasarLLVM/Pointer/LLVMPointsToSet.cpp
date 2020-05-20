/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

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
    : PTA(IRDB, UseLazyEvaluation, PATy) {}

void LLVMPointsToSet::computePointsToSet(const llvm::Value *V) {
  auto *VF = retrieveFunction(V);
  computePointsToSet(VF);
}

void LLVMPointsToSet::computePointsToSet(llvm::Function *F) {
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
  std::vector<size_t> SetsThatNeedReindexing;

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

  // iterate over the worklist, and run the full (n^2)/2 disambiguations
  for (llvm::SetVector<llvm::Value *>::iterator I1 = Pointers.begin(),
                                                E = Pointers.end();
       I1 != E; ++I1) {
    llvm::Type *I1ElTy =
        llvm::cast<llvm::PointerType>((*I1)->getType())->getElementType();
    const uint64_t I1Size = I1ElTy->isSized()
                                ? DL.getTypeStoreSize(I1ElTy)
                                : llvm::MemoryLocation::UnknownSize;
    for (llvm::SetVector<llvm::Value *>::iterator I2 = Pointers.begin();
         I2 != I1; ++I2) {
      llvm::Type *I2ElTy =
          llvm::cast<llvm::PointerType>((*I2)->getType())->getElementType();
      const uint64_t I2Size = I2ElTy->isSized()
                                  ? DL.getTypeStoreSize(I2ElTy)
                                  : llvm::MemoryLocation::UnknownSize;
      switch (AA.alias(*I1, I1Size, *I2, I2Size)) {
      case llvm::NoAlias: {
        // check if those pointers already have a corresponding points-to sets
        auto SearchV1 = PointsToSets.find(*I1);
        if (SearchV1 == PointsToSets.end()) {
          // if not, we need to add a new set
          PointsToSets[*I1] =
              std::make_shared<std::unordered_set<const llvm::Value *>>(
                  std::unordered_set<const llvm::Value *>{*I1});
        }
        auto SearchV2 = PointsToSets.find(*I2);
        if (SearchV2 == PointsToSets.end()) {
          PointsToSets[*I2] =
              std::make_shared<std::unordered_set<const llvm::Value *>>(
                  std::unordered_set<const llvm::Value *>{*I1});
        }
        // if they already have corresponding points-to sets we are fine
        break;
      }
      case llvm::MayAlias: // no break
        [[fallthrough]];
      case llvm::PartialAlias: // no break
        [[fallthrough]];
      case llvm::MustAlias: {
        auto SearchV1 = PointsToSets.find(*I1);
        if (SearchV1 != PointsToSets.end()) {
          SearchV1->second->insert(*I1);
          SearchV1->second->insert(*I2);
        }
        auto SearchV2 = PointsToSets.find(*I2);
        if (SearchV2 != PointsToSets.end()) {
          SearchV2->second->insert(*I1);
          SearchV2->second->insert(*I2);
        }
        // if neither of the pointers has an existing points-to set, we need to
        // add a new one
        if (SearchV1 == PointsToSets.end() && SearchV2 == PointsToSets.end()) {
          auto Pts = std::make_shared<std::unordered_set<const llvm::Value *>>(
              std::unordered_set<const llvm::Value *>{*I1, *I2});
          PointsToSets[*I1] = Pts;
          PointsToSets[*I2] = Pts;
        }
        break;
      }
      }
    }
  }
  // we no longer need the LLVM representation
  // TODO PTA.erase(F);
}

bool LLVMPointsToSet::isInterProcedural() const { return false; }

PointerAnalysisType LLVMPointsToSet::getPointerAnalysistype() const {
  return PTA.getPointerAnalysisType();
}

AliasResult LLVMPointsToSet::alias(const llvm::Value *V1, const llvm::Value *V2,
                                   const llvm::Instruction *I) {
  computePointsToSet(V1);
  computePointsToSet(V2);
  return PointsToSets[V1]->count(V2) ? AliasResult::MustAlias
                                     : AliasResult::NoAlias;
}

const std::unordered_set<const llvm::Value *> &
LLVMPointsToSet::getPointsToSet(const llvm::Value *V,
                                const llvm::Instruction *I) {
  computePointsToSet(V);
  if (PointsToSets.find(V) == PointsToSets.end()) {
    return EmptySet;
  }
  return *PointsToSets[V];
}

std::unordered_set<const llvm::Value *>
LLVMPointsToSet::getReachableAllocationSites(const llvm::Value *V,
                                             const llvm::Instruction *I) {
  computePointsToSet(V);
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
  // TODO
}

void LLVMPointsToSet::introduceAlias(const llvm::Value *V1,
                                     const llvm::Value *V2,
                                     const llvm::Instruction *I) {
  // before introducing additional aliases make sure we initially computed
  // the aliases for V1 and V2
  computePointsToSet(V1);
  computePointsToSet(V2);
  auto SearchV1 = PointsToSets.find(V1);
  auto SearchV2 = PointsToSets.find(V2);
  // better have a safety check
  if (SearchV1 == PointsToSets.end() || SearchV2 == PointsToSets.end()) {
    return;
  }
  auto V1Ptr = SearchV1->first;
  auto V2Ptr = SearchV2->first;
  auto V1Set = SearchV1->second;
  auto V2Set = SearchV2->second;
  // if V1 and V2 are not already aliases, make them aliases
  if (V1Set->find(V2) == V1Set->end()) {
    std::shared_ptr<std::unordered_set<const llvm::Value *>> SmallerSet;
    std::shared_ptr<std::unordered_set<const llvm::Value *>> LargerSet;
    const llvm::Value *SmallerPtr;
    const llvm::Value *LargerPtr;
    if (V1Set->size() < V2Set->size()) {
      SmallerSet = V1Set;
      LargerSet = V2Set;
      SmallerPtr = V1Ptr;
      LargerPtr = V2Ptr;
    } else {
      SmallerSet = V2Set;
      LargerSet = V1Set;
      SmallerPtr = V2Ptr;
      LargerPtr = V1Ptr;
    }
    LargerSet->insert(SmallerSet->begin(), SmallerSet->end());
    // reindex
    for (const auto *Pointer : *SmallerSet) {
      PointsToSets[Pointer] = LargerSet;
    }
    // no we don't need V2Set anymore
    SmallerSet->clear();
    PointsToSets.erase(SmallerPtr);
  }
}

bool LLVMPointsToSet::empty() const { return AnalyzedFunctions.empty(); }

nlohmann::json LLVMPointsToSet::getAsJson() const { return ""_json; }

void LLVMPointsToSet::printAsJson(std::ostream &OS) const {}

void LLVMPointsToSet::print(std::ostream &OS) const {
  size_t NumSets = 0;
  for (const auto &[Ptr, PointsToSetPtr] : PointsToSets) {
    ++NumSets;
    OS << '{';
    size_t NumPointers = 0;
    for (const auto &Pointer : *PointsToSetPtr) {
      ++NumPointers;
      OS << llvmIRToString(Pointer);
      // check for last element
      if (NumPointers != PointsToSetPtr->size()) {
        OS << " <-> ";
      }
    }
    if (NumSets != PointsToSets.size()) {
      OS << "}\n";
    } else {
      OS << '}';
    }
  }
}

} // namespace psr
