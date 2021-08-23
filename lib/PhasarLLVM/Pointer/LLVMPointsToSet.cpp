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
#include <numeric>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseSet.h"
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
  if (SearchV1 == PointsToSets.end()) {
    std::cerr << "ERROR: No points to set for " << llvmIRToString(V1) << "\n";
  }
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

  // reindex the contents of the smaller set
  for (const auto *Ptr : *SmallerSet) {
    PointsToSets[Ptr] = LargerSet;
  }

  // add smaller set to larger one and get rid of the smaller set
  LargerSet->merge(*SmallerSet);

  assert(SmallerSet->empty() && "Expect the points-to-sets to be disjoint");

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

  auto mayAlias = [&AA, &DL](const llvm::Value *V, const llvm::Value *Rep) {
    assert(V->getType()->isPointerTy());
    assert(Rep->getType()->isPointerTy());

    auto VSize =
        V->getType()->getPointerElementType()->isSized()
            ? DL.getTypeStoreSize(V->getType()->getPointerElementType())
            : llvm::MemoryLocation::UnknownSize;

    auto RepSize =
        Rep->getType()->getPointerElementType()->isSized()
            ? DL.getTypeStoreSize(Rep->getType()->getPointerElementType())
            : llvm::MemoryLocation::UnknownSize;

    if (AA.alias(V, VSize, Rep, RepSize) != llvm::AliasResult::NoAlias) {
      return true;
    }

    return false;
  };

  auto addPointer = [this, &mayAlias](const llvm::Value *V, auto &Reps) {
    llvm::SmallVector<unsigned> ToMerge;

    for (unsigned It = 0, End = Reps.size(); It < End; ++It) {
      if (mayAlias(V, Reps[It])) {
        ToMerge.push_back(It);
      }
    }

    if (ToMerge.empty()) {

      Reps.push_back(V);

      addSingletonPointsToSet(V);
    } else if (ToMerge.size() == 1) {
      auto &PTS = PointsToSets[Reps[ToMerge[0]]];
      assert(PTS && "Only added to Reps together with a "
                    "\"addSingletonPointsToSet\" call");

      if (auto VPTS = PointsToSets.find(V); VPTS != PointsToSets.end()) {
        mergePointsToSets(PTS, VPTS->second);
      } else {
        PointsToSets[V] = PTS;
        PTS->insert(V);
      }
    } else {
      auto PTS = PointsToSets[Reps[ToMerge[0]]];
      for (auto Idx : llvm::makeArrayRef(ToMerge).slice(1)) {
        PTS = mergePointsToSets(PTS, PointsToSets[Reps[Idx]]);
      }

      if (auto VPTS = PointsToSets.find(V); VPTS != PointsToSets.end()) {
        mergePointsToSets(PTS, VPTS->second);
      } else {
        PointsToSets[V] = PTS;
        PTS->insert(V);
      }

      Reps.erase(
          remove_by_index(Reps, std::next(ToMerge.begin()), ToMerge.end()),
          Reps.end());
    }
  };

  // llvm::SetVector<const llvm::Value *> Pointers;
  std::vector<const llvm::Value *> Pointers;

  llvm::DenseSet<const llvm::Value *> UsedGlobals;

  auto addIfGlobal = [&](const llvm::Value *Op) {
    llvm::SmallSetVector<const llvm::Value *, 4> WorkList;
    WorkList.insert(Op);

    while (!WorkList.empty()) {
      const auto *Curr = WorkList.pop_back_val();
      if (llvm::isa<llvm::GlobalValue>(Curr)) {
        UsedGlobals.insert(Op);

        // auto PTS = addSingletonPointsToSet(Op);
        // for (const auto *GlobAlias : *PTS) {
        //   if (llvm::isa<llvm::GlobalValue>(GlobAlias) &&
        //       !mayAlias(Op, GlobAlias)) {
        //     /// If they alias also within F, we don't need to check that
        //     again -
        //     /// every pointer that aliases Op, will transitively alias
        //     GlobAlias UsedGlobals.insert(GlobAlias);
        //   }
        // }
        return;
      }
      if (const auto *CurrCE = llvm::dyn_cast<llvm::ConstantExpr>(Curr)) {
        WorkList.insert(CurrCE->value_op_begin(), CurrCE->value_op_end());
      }
    }
  };

  for (auto &Inst : llvm::instructions(F)) {
    if (Inst.getType()->isPointerTy()) {
      // Add all pointer instructions.
      // Pointers.insert(&Inst);
      addPointer(&Inst, Pointers);
      for (auto *Op : Inst.operand_values()) {
        addIfGlobal(Op);
      }
    }

    if (EvalAAMD && llvm::isa<llvm::StoreInst>(&Inst)) {

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
        // Pointers.insert(Callee);
        addPointer(Callee, Pointers);
      }

      /// The operands/arguments of instructions should already be inserted,
      /// because of the SSA form

      // Consider arguments.
      // for (llvm::Use &DataOp : Call->data_ops()) {
      //   if (isInterestingPointer(DataOp)) {
      //     Pointers.insert(DataOp);
      //     // addPointer(DataOp);
      //   }
      // }
    }
    // else {
    // Consider all operands; the instructions we have already seen
    for (auto &Op : Inst.operands()) {
      if (!llvm::isa<llvm::Instruction>(Op) && isInterestingPointer(Op)) {
        // Pointers.insert(Op);
        addPointer(Op, Pointers);
      }
    }
    // }
  }

  for (auto &I : F->args()) {
    if (I.getType()->isPointerTy()) {
      // Add all pointer arguments.
      // Pointers.insert(&I);
      addPointer(&I, Pointers);
    }
  }

  /// Consider globals
  // for (auto &G : F->getParent()->globals()) {
  //   Pointers.insert(&G);
  //   // addPointer(&G);
  // }

  constexpr int KWarningPointers = 100;

  auto NumGlobals = UsedGlobals.size();

  if (Pointers.size() + NumGlobals > KWarningPointers) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), WARNING)
                  << "Large number of pointers detected - Perf is O(N^2) here: "
                  << Pointers.size() << " for "
                  << llvm::demangle(F->getName().str()));

    std::cerr << "Large number of pointers detected - Perf is O(N^2) here: "
              << Pointers.size() << " and " << NumGlobals << " globals for "
              << llvm::demangle(F->getName().str()) << "\n";
    // std::cerr << "> Total " << PTSVector.size() << " points-to-sets\n";
  }

  // std::vector<const llvm::Value *> Representants;
  // Representants.reserve(Pointers.size() + NumGlobals);

  // for (const auto *G : UsedGlobals) {
  //   Representants.push_back(G);
  //   //  addSingletonPointsToSet(G);
  // }

  // if (Pointers.size() + NumGlobals > KWarningPointers) {
  std::cerr << "> added " << Pointers.size()
            << " local pointers; continue with globals pointers\n";
  //}

  // for (const auto *Pointer : Pointers) {
  //   addPointer(Pointer, Representants);
  // }
  Pointers.reserve(Pointers.size() + NumGlobals);
  for (const auto *Glob : UsedGlobals) {
    addPointer(Glob, Pointers);
  }

  // if (Pointers.size() + NumGlobals > KWarningPointers) {
  std::cerr << "> done with globals - reduced to " << Pointers.size() << ";\n";
  //  }

  // for (auto &G : F->getParent()->global_values()) {
  //   addPointer(&G, Representants);
  // }

  if (Pointers.size() + NumGlobals > KWarningPointers) {
    std::cerr << "> done\n";
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
