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
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToAnalysis.h"
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
        // check if those pointers already have corresponding points-to sets
        bool FoundI1 = false;
        bool FoundI2 = false;
        for (auto &PointsToSet : PointsToSets) {
          if (PointsToSet.find(*I1) != PointsToSet.end()) {
            FoundI1 = true;
          }
          if (PointsToSet.find(*I2) != PointsToSet.end()) {
            FoundI2 = true;
          }
        }
        // otherwise create new sets for them
        if (!FoundI1) {
          PointsToSets.push_back({*I1});
        }
        if (!FoundI2) {
          PointsToSets.push_back({*I2});
        }
        break;
      }
      case llvm::MayAlias: // no break
        [[fallthrough]];
      case llvm::PartialAlias: // no break
        [[fallthrough]];
      case llvm::MustAlias: {
        bool FoundPts = false;
        for (auto &PointsToSet : PointsToSets) {
          if (PointsToSet.find(*I1) != PointsToSet.end() ||
              PointsToSet.find(*I2) != PointsToSet.end()) {
            FoundPts = true;
            PointsToSet.insert(*I1);
            PointsToSet.insert(*I2);
            break;
          }
        }
        // add new points-to set if no set has been found
        if (!FoundPts) {
          PointsToSets.push_back({*I1, *I2});
        }
        break;
      }
      }
    }
  }
  // build index
  for (auto &PointsToSet : PointsToSets) {
    for (const auto &Pointer : PointsToSet) {
      if (ValueSetRefMap.find(Pointer) != ValueSetRefMap.end()) {
        // override
        ValueSetRefMap.at(Pointer) = std::reference_wrapper(PointsToSet);
      } else {
        // insert for the first time
        ValueSetRefMap.insert(
            std::make_pair(Pointer, std::reference_wrapper(PointsToSet)));
      }
    }
  }
}

bool LLVMPointsToSet::isInterProcedural() const { return false; }

PointerAnalysisType LLVMPointsToSet::getPointerAnalysistype() const {
  return PTA.getPointerAnalysisType();
}

AliasResult LLVMPointsToSet::alias(const llvm::Value *V1, const llvm::Value *V2,
                                   const llvm::Instruction *I) {
  computePointsToSet(V1);
  computePointsToSet(V2);
  return (ValueSetRefMap.at(V1).get().count(V2)) ? AliasResult::MustAlias
                                                 : AliasResult::NoAlias;
}

const std::unordered_set<const llvm::Value *> &
LLVMPointsToSet::getPointsToSet(const llvm::Value *V,
                                const llvm::Instruction *I) {
  computePointsToSet(V);
  if (ValueSetRefMap.find(V) == ValueSetRefMap.end()) {
    return EmptySet;
  }
  return ValueSetRefMap.at(V);
}

std::unordered_set<const llvm::Value *>
LLVMPointsToSet::getReachableAllocationSites(const llvm::Value *V,
                                             const llvm::Instruction *I) {
  computePointsToSet(V);
  std::unordered_set<const llvm::Value *> AllocSites;
  const auto &PTS = ValueSetRefMap.at(V).get();
  for (const auto *P : PTS) {
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
  auto &V1PTS = ValueSetRefMap.at(V1).get();
  // if V1 and V2 are not already aliases, make them aliases
  if (V1PTS.find(V2) == V1PTS.end()) {
    auto &V2PTS = ValueSetRefMap.at(V2).get();
    auto &Smaller = (V1PTS.size() < V2PTS.size()) ? V1PTS : V2PTS;
    auto &Larger = (V1PTS.size() < V2PTS.size()) ? V2PTS : V1PTS;
    for (auto Pointer : Smaller) {
      Larger.insert(Pointer);
      ValueSetRefMap.at(Pointer) = std::reference_wrapper(Larger);
    }
  }
}

bool LLVMPointsToSet::empty() const { return AnalyzedFunctions.empty(); }

nlohmann::json LLVMPointsToSet::getAsJson() const { return ""_json; }

void LLVMPointsToSet::printAsJson(std::ostream &OS) const {}

void LLVMPointsToSet::print(std::ostream &OS) const {
  for (const auto &PointsToSet : PointsToSets) {
    OS << '{';
    size_t NumPointers = 0;
    for (const auto &Pointer : PointsToSet) {
      ++NumPointers;
      OS << llvmIRToString(Pointer);
      // check for last element
      if (NumPointers != PointsToSet.size()) {
        OS << ", ";
      }
    }
    if (PointsToSet != PointsToSets.back()) {
      OS << "}, ";
    } else {
      OS << '}';
    }
  }
}

} // namespace psr
