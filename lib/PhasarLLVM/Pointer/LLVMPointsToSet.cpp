/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
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
  for (llvm::Module *M : IRDB.getAllModules()) {
    // compute points-to information for all globals

    for (const auto &G : M->globals()) {
      computeValuesPointsToSet(&G);
    }

    for (const auto &F : M->functions()) {
      computeValuesPointsToSet(&F);
    }

    if (!UseLazyEvaluation) {

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
        // The may be no corresponding function when the instruction is used in
        // a vtable, for instance.
        if (Inst->getParent()) {
          computeFunctionsPointsToSet(
              const_cast<llvm::Function *>(Inst->getFunction()));
          if (!llvm::isa<llvm::Function>(G) && isInterestingPointer(User)) {
            mergePointsToSets(User, G);
          } else if (const auto *Store =
                         llvm::dyn_cast<llvm::StoreInst>(User)) {
            if (isInterestingPointer(Store->getValueOperand())) {
              // Store->getPointerOperand() doesn't require checking: it is
              // always an interesting pointer
              mergePointsToSets(Store->getValueOperand(),
                                Store->getPointerOperand());
            }
          }
        }
      }
    }

  } else {
    const auto *VF = retrieveFunction(V);
    computeFunctionsPointsToSet(const_cast<llvm::Function *>(VF));
  }
}

auto LLVMPointsToSet::addSingletonPointsToSet(const llvm::Value *V)
    -> PointsToSetPtrTy {

  auto &PTS = PointsToSets[V];

  if (!PTS) {
    PTS = std::make_shared<std::unordered_set<const llvm::Value *>>(
        std::unordered_set<const llvm::Value *>{V});
  }

  assert(PTS->count(V));
  return PTS;
}

void LLVMPointsToSet::mergePointsToSets(const llvm::Value *V1,
                                        const llvm::Value *V2) {
  if (V1 == V2) {
    return;
  }

  auto SearchV1 = PointsToSets.find(V1);
  assert(SearchV1 != PointsToSets.end());

  auto SearchV2 = PointsToSets.find(V2);
  assert(SearchV2 != PointsToSets.end());

  mergePointsToSets(SearchV1->second, SearchV2->second);
}

auto LLVMPointsToSet::mergePointsToSets(const PointsToSetPtrTy &PTS1,
                                        const PointsToSetPtrTy &PTS2)
    -> PointsToSetPtrTy {
  if (PTS1 == PTS2) {
    return PTS1;
  }

  assert(PTS1);
  assert(PTS2);

  auto [SmallerSet, LargerSet] = [&]() {
    if (PTS1->size() > PTS2->size()) {
      return std::make_pair(PTS2, PTS1);
    }
    return std::make_pair(PTS1, PTS2);
  }();

  // add smaller set to larger one
  LargerSet->insert(SmallerSet->begin(), SmallerSet->end());
  // reindex the contents of the smaller set
  for (const auto *Ptr : *SmallerSet) {
    PointsToSets[Ptr] = LargerSet;
  }
  // get rid of the smaller set
  SmallerSet->clear();

  return LargerSet;
}

bool LLVMPointsToSet::interIsReachableAllocationSiteTy(const llvm::Value *V,
                                                       const llvm::Value *P) {
  // consider the full inter-procedural points-to/alias information

  if (llvm::isa<llvm::AllocaInst>(P)) {
    return true;
  }
  if (llvm::isa<llvm::CallInst>(P) || llvm::isa<llvm::InvokeInst>(P)) {
    const llvm::CallBase *CS = llvm::dyn_cast<llvm::CallBase>(P);
    if (CS->getCalledFunction() != nullptr &&
        CS->getCalledFunction()->hasName() &&
        HeapAllocatingFunctions.count(CS->getCalledFunction()->getName())) {
      return true;
    }
  }

  return false;
}

bool LLVMPointsToSet::intraIsReachableAllocationSiteTy(
    const llvm::Value *V, const llvm::Value *P, const llvm::Function *VFun,
    const llvm::GlobalObject *VG) {
  // consider the function-local, i.e. intra-procedural, points-to/alias
  // information only

  // We may not be able to retrieve a function for the given value since some
  // pointer values can exist outside functions, for instance, in case of
  // vtables, etc.
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(P)) {
    // only add function local allocation sites
    if ((VFun && VFun == Alloca->getFunction())) {
      return true;
    }
    if (VG) {
      return true;
    }
  } else if (llvm::isa<llvm::CallInst>(P) || llvm::isa<llvm::InvokeInst>(P)) {
    const llvm::CallBase *CS = llvm::dyn_cast<llvm::CallBase>(P);
    if (CS->getCalledFunction() != nullptr &&
        CS->getCalledFunction()->hasName() &&
        HeapAllocatingFunctions.count(CS->getCalledFunction()->getName())) {
      if (VFun && VFun == CS->getFunction()) {
        return true;
      } else if (VG) {
        return true;
      }
    }
  }

  return false;
}

void LLVMPointsToSet::computeFunctionsPointsToSet(llvm::Function *F) {
  // F may be null
  if (!F) {
    return;
  }
  // check if we already analyzed the function
  if (auto [Unused, Inserted] = AnalyzedFunctions.insert(F);
      !Inserted || F->isDeclaration()) {
    return;
  }
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "Analyzing function: " << F->getName().str());

  llvm::AAResults &AA = *PTA.getAAResults(F);
  bool EvalAAMD = true;

  // taken from llvm/Analysis/AliasAnalysisEvaluator.cpp
  const llvm::DataLayout &DL = F->getParent()->getDataLayout();

  llvm::SetVector<const llvm::Value *> Pointers;

  for (auto &Inst : llvm::instructions(F)) {
    if (Inst.getType()->isPointerTy()) {
      // Add all pointer instructions.
      Pointers.insert(&Inst);
    }
    // if (EvalAAMD && llvm::isa<llvm::LoadInst>(&*I)) {
    //   Loads.insert(&*I);
    // }
    if (EvalAAMD && llvm::isa<llvm::StoreInst>(&Inst)) {
      //  Stores.insert(&*I);
      auto *Store = llvm::cast<llvm::StoreInst>(&Inst);
      auto *SVO = Store->getValueOperand();
      auto *SPO = Store->getPointerOperand();
      if (SVO->getType()->isPointerTy()) {

        if (llvm::isa<llvm::Function>(SVO)) {
          addSingletonPointsToSet(SVO);
          addSingletonPointsToSet(SPO);
          mergePointsToSets(SVO, SPO);
        }
        if (auto *SVOCE = llvm::dyn_cast<llvm::ConstantExpr>(SVO)) {
          if (SVOCE->isCast()) {
            auto *RHS = SVOCE->getOperand(0);

            addSingletonPointsToSet(SPO);
            if (RHS->getType()->isPointerTy()) {
              addSingletonPointsToSet(RHS);
              mergePointsToSets(RHS, SPO);
            }

            addSingletonPointsToSet(SVOCE);
            mergePointsToSets(SVOCE, SPO);
          }
        }
      }
    }

    if (auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
      llvm::Value *Callee = Call->getCalledOperand();
      // Skip actual functions for direct function calls.
      if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee)) {
        Pointers.insert(Callee);
      }
      // Consider arguments.
      for (llvm::Use &DataOp : Call->data_ops()) {
        if (isInterestingPointer(DataOp)) {
          Pointers.insert(DataOp);
        }
      }
      // Calls.insert(Call);
    } else {
      // Consider all operands.
      for (auto &Op : Inst.operands()) {
        if (isInterestingPointer(Op)) {
          Pointers.insert(Op);
        }
      }
    }
  }

  for (auto &I : F->args()) {
    if (I.getType()->isPointerTy()) {
      // Add all pointer arguments.
      Pointers.insert(&I);
    }
  }

  /// Consider globals
  for (auto &G : F->getParent()->globals()) {
    Pointers.insert(&G);
  }

  auto PTSVector = [&] {
    std::unordered_set<std::shared_ptr<std::unordered_set<const llvm::Value *>>>
        InterestingPTS;
    std::vector<std::shared_ptr<std::unordered_set<const llvm::Value *>>>
        PTSVector;

    InterestingPTS.reserve(Pointers.size() / 2);
    PTSVector.reserve(Pointers.size() / 2);
    // Introduce a singleton set for each pointer.
    // Those sets will be merged as we discover aliases
    for (const auto *Pointer : Pointers) {
      auto [It, Inserted] =
          InterestingPTS.insert(addSingletonPointsToSet(Pointer));
      if (Inserted) {
        PTSVector.push_back(*It);
      }
    }

    return PTSVector;
  }();

  auto getSizeOf = [&DL](const llvm::Value *V) {
    if (!V->getType()->isPointerTy()) {
      std::cerr << "ERROR: " << llvmIRToString(V) << " is no pointer!\n";
    }
    llvm::Type *I1ElTy = V->getType()->getPointerElementType();
    return I1ElTy->isSized() ? DL.getTypeStoreSize(I1ElTy)
                             : llvm::MemoryLocation::UnknownSize;
  };
  auto mayAlias = [&AA](const llvm::Value *V1, uint64_t V1Size,
                        const llvm::Value *V2, uint64_t V2Size) {
    return AA.alias(V1, V1Size, V2, V2Size) != llvm::AliasResult::NoAlias;
  };

  constexpr int KWarningPointers = 100;
  if (Pointers.size() > KWarningPointers) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), WARNING)
                  << "Large number of pointers detected - Perf is O(N^2) here: "
                  << Pointers.size() << " for "
                  << llvm::demangle(F->getName().str()));

    std::cerr << "Large number of pointers detected - Perf is O(N^2) here: "
              << Pointers.size() << " for "
              << llvm::demangle(F->getName().str()) << "\n";
    std::cerr << "> Total " << PTSVector.size() << " points-to-sets\n";
  }

  llvm::SmallVector<size_t> ToMerge;

  size_t ctr = 0;
  size_t skipCtr = 0;
  for (size_t I1 = 0; I1 < PTSVector.size(); ++I1) {
    auto PTS1 = PTSVector[I1];
    assert(PTS1);

    for (size_t I2 = I1 + 1; I2 < PTSVector.size(); ++I2) {
      /// Assuming, the mayAlias relation is transitive, we can start with
      /// I2=I1+1

      auto PTS2 = PTSVector[I2];
      assert(PTS2);

      for (const auto *V1 : *PTS1) {
        const auto *F1 = retrieveFunction(V1);
        if (F1 && F1 != F) {
          ++skipCtr;
          ++ctr;
          continue;
        }
        auto V1Size = getSizeOf(V1);
        bool EarlyBreak = false;

        for (const auto *V2 : *PTS2) {
          ++ctr;
          const auto *F2 = retrieveFunction(V2);
          if (F2 && F2 != F) {
            ++skipCtr;
            continue;
          }

          if (mayAlias(V1, V1Size, V2, getSizeOf(V2))) {
            EarlyBreak = true;
            ToMerge.push_back(I2);
            break;
          }
        }
        if (EarlyBreak) {
          break;
        }
      }
    }

    if (!ToMerge.empty()) {
      size_t Diff = 0;
      for (auto MergeIdx : ToMerge) {
        PTS1 = mergePointsToSets(PTS1, PTSVector[MergeIdx]);
        PTSVector[MergeIdx] = nullptr;
        if (MergeIdx < I1) {
          ++Diff;
        }
      }

      PTSVector.erase(std::remove_if(PTSVector.begin(), PTSVector.end(),
                                     [](auto &&PTS) { return PTS == nullptr; }),
                      PTSVector.end());

      I1 -= Diff;

      ToMerge.clear();
    }
  }

  // // iterate over the worklist, and run the full (n^2)/2 disambiguations
  // for (auto I1 = Pointers.begin(), E = Pointers.end(); I1 != E; ++I1) {

  //   const uint64_t I1Size = getSizeOf(*I1);
  //   for (auto I2 = Pointers.begin(); I2 != I1; ++I2) {
  //     const uint64_t I2Size = getSizeOf(*I2);

  //     if (mayAlias(*I1, I1Size, *I2, I2Size)) {
  //       mergePointsToSets(*I1, *I2);
  //     }
  //   }
  // }

  if (Pointers.size() > KWarningPointers) {
    std::cerr << "> done; skipped " << skipCtr << "/" << ctr << " pointers\n";
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
  static std::shared_ptr<std::unordered_set<const llvm::Value *>> EmptySet =
      std::make_shared<std::unordered_set<const llvm::Value *>>();

  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return EmptySet;
  }
  // compute V's points-to set
  computeValuesPointsToSet(V);
  if (auto It = PointsToSets.find(V); It != PointsToSets.end()) {
    return It->second;
  }
  // if we still can't find its value return an empty set
  return EmptySet;
}

std::shared_ptr<std::unordered_set<const llvm::Value *>>
LLVMPointsToSet::getReachableAllocationSites(const llvm::Value *V,
                                             bool IntraProcOnly,
                                             const llvm::Instruction *I) {
  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return std::make_shared<std::unordered_set<const llvm::Value *>>();
  }
  computeValuesPointsToSet(V);
  auto AllocSites = std::make_shared<std::unordered_set<const llvm::Value *>>();
  const auto PTS = PointsToSets[V];
  // consider the full inter-procedural points-to/alias information
  if (!IntraProcOnly) {
    for (const auto *P : *PTS) {
      if (interIsReachableAllocationSiteTy(V, P)) {
        AllocSites->insert(P);
      }
    }
  } else {
    // consider the function-local, i.e. intra-procedural, points-to/alias
    // information only
    const auto *VFun = retrieveFunction(V);
    const auto *VG = llvm::dyn_cast<llvm::GlobalObject>(V);
    // We may not be able to retrieve a function for the given value since some
    // pointer values can exist outside functions, for instance, in case of
    // vtables, etc.
    for (const auto *P : *PTS) {
      if (intraIsReachableAllocationSiteTy(V, P, VFun, VG)) {
        AllocSites->insert(P);
      }
    }
  }
  return AllocSites;
}

bool LLVMPointsToSet::isInReachableAllocationSites(
    const llvm::Value *V, const llvm::Value *PotentialValue, bool IntraProcOnly,
    const llvm::Instruction *I) {
  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return false;
  }
  computeValuesPointsToSet(V);

  bool PVIsReachableAllocationSiteType = false;
  if (IntraProcOnly) {
    const auto *VFun = retrieveFunction(V);
    const auto *VG = llvm::dyn_cast<llvm::GlobalObject>(V);
    PVIsReachableAllocationSiteType =
        intraIsReachableAllocationSiteTy(V, PotentialValue, VFun, VG);
  } else {
    PVIsReachableAllocationSiteType =
        interIsReachableAllocationSiteTy(V, PotentialValue);
  }

  if (PVIsReachableAllocationSiteType) {
    const auto PTS = PointsToSets[V];
    return PTS->count(PotentialValue);
  }

  return false;
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

void LLVMPointsToSet::peakIntoPointsToSet(
    const PointsToSetMap::value_type &ValueSetPair, int Peak) {
  llvm::outs() << "Value: ";
  ValueSetPair.first->print(llvm::outs());
  llvm::outs() << '\n';
  int PeakCounter = 0;
  llvm::outs() << "aliases with: {\n";
  for (const llvm::Value *I : *ValueSetPair.second) {
    I->print(llvm::outs());
    llvm::outs() << '\n';
    PeakCounter++;
    if (PeakCounter > Peak) {
      llvm::outs() << llvm::formatv("... and {0} more\n",
                                    ValueSetPair.second->size() - Peak);
      break;
    }
  }
  llvm::outs() << "}\n";
}

void LLVMPointsToSet::drawPointsToSetsDistribution(int Peak) const {
  std::vector<std::pair<size_t, unsigned>> SizeAmountPairs;

  for (const auto &ValueSetPair : PointsToSets) {
    auto Search =
        std::find_if(SizeAmountPairs.begin(), SizeAmountPairs.end(),
                     [&ValueSetPair](const auto &Entry) {
                       return Entry.first == ValueSetPair.second->size();
                     });
    if (Search != SizeAmountPairs.end()) {
      Search->second++;
    } else {
      SizeAmountPairs.emplace_back(ValueSetPair.second->size(), 1);
    }
  }

  std::sort(SizeAmountPairs.begin(), SizeAmountPairs.end(),
            [](const auto &KVPair1, const auto &KVPair2) {
              return KVPair1.first < KVPair2.first;
            });

  int TotalValues = std::accumulate(
      SizeAmountPairs.begin(), SizeAmountPairs.end(), 0,
      [](int Current, const auto &KVPair) { return Current + KVPair.second; });

  llvm::outs() << llvm::formatv("{0,10}  {1,-=50} {2,10}\n", "PtS Size",
                                "Distribution", "Number of sets");
  for (auto &KV : SizeAmountPairs) {
    std::string PeakBar(static_cast<double>(KV.second) * 50 /
                            static_cast<double>(TotalValues),
                        '*');
    llvm::outs() << llvm::formatv("{0,10} |{1,-50} {2,-10}\n", KV.first,
                                  PeakBar, KV.second);
  }
  llvm::outs() << "\n";

  if (Peak) {
    for (const auto &ValueSetPair : PointsToSets) {
      if (ValueSetPair.second->size() == SizeAmountPairs.back().first) {
        llvm::outs() << "Peak into one of the biggest points sets.\n";
        peakIntoPointsToSet(ValueSetPair, Peak);
        return;
      }
    }
  }
}

} // namespace psr
