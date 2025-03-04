/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Utils/BoxedPointer.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/TypeSize.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace psr {

template class BoxedPtr<LLVMAliasInfo::AliasSetTy>;
template class BoxedConstPtr<LLVMAliasInfo::AliasSetTy>;
template class AliasSetOwner<LLVMAliasInfo::AliasSetTy>;

LLVMAliasSet::LLVMAliasSet(LLVMProjectIRDB *IRDB, bool UseLazyEvaluation,
                           AliasAnalysisType PATy)
    : PTA(*IRDB, UseLazyEvaluation, PATy) {
  assert(IRDB != nullptr);

  auto NumGlobals = IRDB->getNumGlobals();
  AliasSets.reserve(NumGlobals);
  Owner.reserve(NumGlobals);

  PHASAR_LOG_LEVEL_CAT(
      INFO, "LLVMAliasSet",
      "Start constructing LLVMAliasSet "
          << std::chrono::steady_clock::now().time_since_epoch().count());
  auto *M = IRDB->getModule();

  // compute points-to information for all globals

  for (const auto &G : M->globals()) {
    computeValuesAliasSet(&G);
  }

  for (const auto &F : M->functions()) {
    computeValuesAliasSet(&F);
  }

  if (!UseLazyEvaluation) {
    // compute points-to information for all functions
    for (auto &F : *M) {
      if (!F.isDeclaration()) {
        computeFunctionsAliasSet(&F);
      }
    }
  }

  PHASAR_LOG_LEVEL_CAT(
      INFO, "LLVMAliasSet",
      "LLVMAliasSet completed "
          << std::chrono::steady_clock::now().time_since_epoch().count());
}

LLVMAliasSet::LLVMAliasSet(LLVMProjectIRDB *IRDB,
                           const nlohmann::json &SerializedPTS)
    : PTA(*IRDB, true) {
  assert(IRDB != nullptr);
  // Assume, we already have validated the json schema

  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMAliasSet",
                       "Load precomputed points-to info from JSON");

  const auto &Sets = SerializedPTS.at("AliasSets");
  assert(Sets.is_array());
  const auto &Fns = SerializedPTS.at("AnalyzedFunctions");
  assert(Fns.is_array());

  /// Deserialize the AliasSets - an array of arrays (both are to be
  /// interpreted as sets of metadata-ids)

  Owner.reserve(Sets.size());
  for (const auto &PtsJson : Sets) {
    assert(PtsJson.is_array());
    auto PTS = Owner.acquire();
    for (const auto &Alias : PtsJson) {
      const auto AliasStr = Alias.get<std::string>();
      const auto *Inst = fromMetaDataId(*IRDB, AliasStr);
      if (!Inst) {
        PHASAR_LOG_LEVEL(WARNING, "Invalid Value-Id: " << AliasStr);
        continue;
      }

      AliasSets[Inst] = PTS;
      PTS->insert(Inst);
    }
  }

  /// Deserialize the AnalyzedFunctions - an array of function-names (to be
  /// interpreted as set)

  AnalyzedFunctions.reserve(Fns.size());
  for (const auto &F : Fns) {
    if (!F.is_string()) {
      PHASAR_LOG_LEVEL(WARNING, "Invalid Function Name: " << F);
      continue;
    }

    const auto *IRFn = IRDB->getFunction(F.get<std::string>());

    if (!IRFn) {
      PHASAR_LOG_LEVEL(WARNING, "Function: " << F << " not in the IRDB");
      continue;
    }

    AnalyzedFunctions.insert(IRFn);
  }
}

void LLVMAliasSet::computeValuesAliasSet(const llvm::Value *V) {
  if (!isInterestingPointer(V)) {
    // don't need to do anything
    return;
  }
  // Add set for the queried value if none exists, yet
  addSingletonAliasSet(V);
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

          computeFunctionsAliasSet(
              const_cast<llvm::Function *> // NOLINT - FIXME when it is fixed in
                                           // LLVM
              (Inst->getFunction()));
          if (!llvm::isa<llvm::Function>(G) && isInterestingPointer(User)) {
            mergeAliasSets(User, G);
          } else if (const auto *Store =
                         llvm::dyn_cast<llvm::StoreInst>(User)) {
            if (isInterestingPointer(Store->getValueOperand())) {
              // Store->getPointerOperand() doesn't require checking: it is
              // always an interesting pointer
              mergeAliasSets(Store->getValueOperand(),
                             Store->getPointerOperand());
            }
          }
        }
      }
    }

  } else {
    const auto *VF = retrieveFunction(V);
    computeFunctionsAliasSet(
        const_cast<llvm::Function *> // NOLINT - FIXME when it is fixed in LLVM
        (VF));
  }
}

void LLVMAliasSet::addSingletonAliasSet(const llvm::Value *V) {
  auto [It, Inserted] = AliasSets.try_emplace(V, nullptr);

  if (!Inserted) {
    assert(It->second->count(V));
    return;
  }

  auto PTS = Owner.acquire();
  PTS->insert(V);
  It->second = PTS;
}

void LLVMAliasSet::mergeAliasSets(const llvm::Value *V1,
                                  const llvm::Value *V2) {
  if (V1 == V2) {
    return;
  }

  auto SearchV1 = AliasSets.find(V1);
  assert(SearchV1 != AliasSets.end());

  auto SearchV2 = AliasSets.find(V2);
  assert(SearchV2 != AliasSets.end());

  mergeAliasSets(SearchV1->second, SearchV2->second);
}

void LLVMAliasSet::mergeAliasSets(BoxedPtr<AliasSetTy> PTS1,
                                  BoxedPtr<AliasSetTy> PTS2) {
  if (PTS1 == PTS2 || PTS1.get() == PTS2.get()) {
    return;
  }

  assert(PTS1 && PTS1.get());
  assert(PTS2 && PTS2.get());

  auto [SmallerSet, LargerSet] = [&]() {
    if (PTS1->size() > PTS2->size()) {
      return std::make_pair(PTS2, PTS1);
    }
    return std::make_pair(PTS1, PTS2);
  }();

  // reindex the contents of the smaller set
  // for (const auto *Ptr : *SmallerSet) {
  //   AliasSets[Ptr] = LargerSet;
  // }

  // add smaller set to larger one and get rid of the smaller set
  LargerSet->reserve(LargerSet->size() + SmallerSet->size());
  LargerSet->insert(SmallerSet->begin(), SmallerSet->end());

  // *SmallerSet.value() = LargerSet.get();
  auto *ToDelete = SmallerSet.get();
  for (const auto *Vals : *SmallerSet) {
    auto PTS = AliasSets[Vals];
    if (PTS.get() == ToDelete) {

      *PTS.value() = LargerSet.get();
    }
  }

  Owner.release(ToDelete);
}

bool LLVMAliasSet::interIsReachableAllocationSiteTy(
    [[maybe_unused]] const llvm::Value *V, const llvm::Value *P) const {
  // consider the full inter-procedural points-to/alias information

  if (llvm::isa<llvm::AllocaInst>(P)) {
    return true;
  }
  if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(P)) {
    if (CS->getCalledFunction() != nullptr &&
        isHeapAllocatingFunction(CS->getCalledFunction())) {
      return true;
    }
  }

  return false;
}

bool LLVMAliasSet::intraIsReachableAllocationSiteTy(
    [[maybe_unused]] const llvm::Value *V, const llvm::Value *P,
    const llvm::Function *VFun, const llvm::GlobalObject *VG) const {
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
  } else if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(P)) {

    if (CS->getCalledFunction() != nullptr &&
        isHeapAllocatingFunction(CS->getCalledFunction())) {
      if (VFun && VFun == CS->getFunction()) {
        return true;
      }
      if (VG) {
        return true;
      }
    }
  } else if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(P)) {
    /// Arguments are no allocation sites, but in the inTRAprocedural case, they
    /// act as such
    return Arg->getParent() == VFun;
  }

  return false;
}

static llvm::Type *getPointeeTypeOrNull(const llvm::Value *Ptr) {
  assert(Ptr->getType()->isPointerTy());

  if (!Ptr->getType()->isOpaquePointerTy()) {
    return Ptr->getType()->getNonOpaquePointerElementType();
  }

  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Ptr)) {
    if (auto *Ty = Arg->getParamByValType()) {
      return Ty;
    }
    if (auto *Ty = Arg->getParamStructRetType()) {
      return Ty;
    }
  }
  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Ptr)) {
    return Alloca->getAllocatedType();
  }
  return nullptr;
}

static bool mayAlias(llvm::AAResults &AA, const llvm::DataLayout &DL,
                     const llvm::Value *V, const llvm::Value *Rep) {
  assert(V->getType()->isPointerTy());
  assert(Rep->getType()->isPointerTy());

  auto *ElTy = getPointeeTypeOrNull(V);
  auto *RepElTy = getPointeeTypeOrNull(Rep);

  auto VSize = ElTy && ElTy->isSized()
                   ? llvm::LocationSize::precise(DL.getTypeStoreSize(ElTy))
                   : llvm::LocationSize::precise(1);

  auto RepSize = RepElTy && RepElTy->isSized()
                     ? llvm::LocationSize::precise(DL.getTypeStoreSize(RepElTy))
                     : llvm::LocationSize::precise(1);

  if (AA.alias(V, VSize, Rep, RepSize) != llvm::AliasResult::NoAlias) {
    return true;
  }

  return false;
}

void LLVMAliasSet::addPointer(llvm::AAResults &AA, const llvm::DataLayout &DL,
                              const llvm::Value *V,
                              std::vector<const llvm::Value *> &Reps) {
  llvm::SmallVector<unsigned> ToMerge;

  for (unsigned It = 0, End = Reps.size(); It < End; ++It) {
    if (mayAlias(AA, DL, V, Reps[It])) {
      ToMerge.push_back(It);
    }
  }

  // If we find several alias sets that may alias V, we must merge them.
  // However, we cannot simply remove all respective representants (except for
  // one), because for interprocedural flows, LLVM assumes all possible
  // aliases and i8* is treated as void* making every pointer-type a matching
  // type.
  // This destroys the transitivity of the mayAlias relation partially. We can
  // still remove a representant, if we have another rep of the same type
  // within the same alias set.

  if (ToMerge.empty()) {
    Reps.push_back(V);
    addSingletonAliasSet(V);
  } else if (ToMerge.size() == 1) {
    auto PTS = AliasSets[Reps[ToMerge[0]]];
    assert(PTS && "Only added to Reps together with a "
                  "\"addSingletonAliasSet\" call");

    if (auto VPTS = AliasSets.find(V); VPTS != AliasSets.end()) {
      mergeAliasSets(PTS, VPTS->second);
    } else {
      AliasSets[V] = PTS;
      PTS->insert(V);
    }

    if (V->getType() != Reps[ToMerge[0]]->getType()) {
      Reps.push_back(V);
    }

  } else {
    auto PTS = AliasSets[Reps[ToMerge[0]]];
    llvm::SmallPtrSet<const llvm::Type *, 6> OccurringTypes{
        Reps[ToMerge[0]]->getType()};
    llvm::SmallVector<unsigned> ToRemove;

    for (auto Idx : llvm::makeArrayRef(ToMerge).slice(1)) {
      mergeAliasSets(PTS, AliasSets[Reps[Idx]]);
      if (auto [Unused, Inserted] = OccurringTypes.insert(Reps[Idx]->getType());
          !Inserted) {
        ToRemove.push_back(Idx);
      }
    }

    if (auto VPTS = AliasSets.find(V); VPTS != AliasSets.end()) {
      mergeAliasSets(PTS, VPTS->second);
    } else {
      AliasSets[V] = PTS;
      PTS->insert(V);
    }

    Reps.erase(remove_by_index(Reps, ToRemove.begin(), ToRemove.end()),
               Reps.end());

    if (auto [Unused, Inserted] = OccurringTypes.insert(V->getType());
        Inserted) {
      Reps.push_back(V);
    }
  }
}

static void addIfGlobal(llvm::DenseSet<const llvm::Value *> &UsedGlobals,
                        const llvm::Value *Op) {
  llvm::SmallPtrSet<const llvm::Value *, 4> Seen;
  llvm::SmallVector<const llvm::Value *, 4> WorkList;
  WorkList.push_back(Op);
  Seen.insert(Op);

  while (!WorkList.empty()) {
    const auto *Curr = WorkList.pop_back_val();

    if (llvm::isa<llvm::GlobalObject>(Curr)) {
      UsedGlobals.insert(Curr);

      if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(Curr);
          Glob && Glob->hasInitializer() &&
          Seen.insert(Glob->getInitializer()).second) {
        WorkList.push_back(Glob->getInitializer());
      }

    } else if (llvm::isa<llvm::ConstantExpr>(Curr) ||
               llvm::isa<llvm::ConstantAggregate>(Curr)) {
      for (const auto &CEOp : llvm::cast<llvm::User>(Curr)->operands()) {
        if (Seen.insert(CEOp).second) {
          WorkList.push_back(CEOp);
        }
      }
    }
  }
}

void LLVMAliasSet::computeFunctionsAliasSet(llvm::Function *F) {
  // F may be null
  if (!F) {
    return;
  }
  // check if we already analyzed the function
  if (auto [Unused, Inserted] = AnalyzedFunctions.insert(F);
      !Inserted || F->isDeclaration()) {
    return;
  }
  PHASAR_LOG_LEVEL_CAT(DEBUG, "LLVMAliasSet",
                       "Analyzing function: " << F->getName());

  llvm::AAResults &AA = *PTA.getAAResults(F);
  bool EvalAAMD = true;

  const llvm::DataLayout &DL = F->getParent()->getDataLayout();

  auto addPointer = [this, &AA, &DL](const llvm::Value *V, // NOLINT
                                     std::vector<const llvm::Value *> &Reps) {
    return this->addPointer(AA, DL, V, Reps);
  };

  std::vector<const llvm::Value *> Pointers;
  llvm::DenseSet<const llvm::Value *> UsedGlobals;

  for (auto &Inst : llvm::instructions(F)) {
    if (Inst.getType()->isPointerTy()) {
      // Add all pointer instructions.
      addPointer(&Inst, Pointers);
    }

    if (EvalAAMD && llvm::isa<llvm::StoreInst>(&Inst)) {

      auto *Store = llvm::cast<llvm::StoreInst>(&Inst);
      auto *SVO = Store->getValueOperand();
      auto *SPO = Store->getPointerOperand();
      if (SVO->getType()->isPointerTy()) {

        if (llvm::isa<llvm::Function>(SVO)) {
          addSingletonAliasSet(SVO);
          addSingletonAliasSet(SPO);
          mergeAliasSets(SVO, SPO);
        }
        if (auto *SVOCE = llvm::dyn_cast<llvm::ConstantExpr>(SVO)) {
          if (SVOCE->isCast()) {
            auto *RHS = SVOCE->getOperand(0);

            addSingletonAliasSet(SPO);
            if (RHS->getType()->isPointerTy()) {
              addSingletonAliasSet(RHS);
              mergeAliasSets(RHS, SPO);
            }

            addSingletonAliasSet(SVOCE);
            mergeAliasSets(SVOCE, SPO);
          }
        }
      }
    }

    /// The operands/arguments of instructions should already be inserted,
    /// because of the SSA form

    if (auto *Call = llvm::dyn_cast<llvm::CallBase>(&Inst)) {
      llvm::Value *Callee = Call->getCalledOperand();
      // Skip actual functions for direct function calls.
      if (!llvm::isa<llvm::Function>(Callee) && isInterestingPointer(Callee) &&
          !llvm::isa<llvm::Instruction>(Callee)) {
        addPointer(Callee, Pointers);
      }

      // Consider arguments.
      for (llvm::Use &DataOp : Call->data_ops()) {
        addIfGlobal(UsedGlobals, DataOp);
        if (!llvm::isa<llvm::Instruction>(DataOp) &&
            isInterestingPointer(DataOp)) {
          addPointer(DataOp, Pointers);
        }
      }
    } else {
      // Consider all operands; the instructions we have already seen
      for (auto &Op : Inst.operands()) {
        addIfGlobal(UsedGlobals, Op);
        if (!llvm::isa<llvm::Instruction>(Op) && isInterestingPointer(Op)) {
          addPointer(Op, Pointers);
        }
      }
    }
  }

  for (auto &I : F->args()) {
    if (I.getType()->isPointerTy()) {
      // Add all pointer arguments.
      addPointer(&I, Pointers);
    }
  }

  auto NumGlobals = UsedGlobals.size();

  Pointers.reserve(Pointers.size() + NumGlobals);
  for (const auto *Glob : UsedGlobals) {
    addPointer(Glob, Pointers);
  }

  // we no longer need the LLVM representation
  PTA.erase(F);
}

AliasResult LLVMAliasSet::alias(const llvm::Value *V1, const llvm::Value *V2,
                                [[maybe_unused]] const llvm::Instruction *I) {
  // if V1 or V2 is not an interesting pointer those values cannot alias
  if (!isInterestingPointer(V1) || !isInterestingPointer(V2)) {
    return AliasResult::NoAlias;
  }
  computeValuesAliasSet(V1);
  computeValuesAliasSet(V2);
  return AliasSets[V1]->count(V2) ? AliasResult::MayAlias
                                  : AliasResult::NoAlias;
}

auto LLVMAliasSet::getEmptyAliasSet() -> BoxedPtr<AliasSetTy> {
  static AliasSetTy EmptySet{};
  static AliasSetTy *EmptySetPtr = &EmptySet;
  return &EmptySetPtr;
}

auto LLVMAliasSet::getAliasSet(const llvm::Value *V,
                               [[maybe_unused]] const llvm::Instruction *I)
    -> AliasSetPtrTy {

  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return getEmptyAliasSet();
  }
  // compute V's points-to set
  computeValuesAliasSet(V);
  if (auto It = AliasSets.find(V); It != AliasSets.end()) {
    return It->second;
  }
  // if we still can't find its value return an empty set
  return getEmptyAliasSet();
}

auto LLVMAliasSet::getReachableAllocationSites(
    const llvm::Value *V, bool IntraProcOnly,
    [[maybe_unused]] const llvm::Instruction *I) -> AllocationSiteSetPtrTy {

  auto AllocSites = std::make_unique<AliasSetTy>();

  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return AllocSites;
  }
  computeValuesAliasSet(V);

  const auto PTS = AliasSets[V];
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

bool LLVMAliasSet::isInReachableAllocationSites(
    const llvm::Value *V, const llvm::Value *PotentialValue, bool IntraProcOnly,
    [[maybe_unused]] const llvm::Instruction *I) {
  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return false;
  }
  computeValuesAliasSet(V);

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
    const auto PTS = AliasSets[V];
    return PTS->count(PotentialValue);
  }

  return false;
}

void LLVMAliasSet::mergeWith(const LLVMAliasSet &OtherPTI) {

  // merge analyzed functions
  AnalyzedFunctions.insert(OtherPTI.AnalyzedFunctions.begin(),
                           OtherPTI.AnalyzedFunctions.end());
  // merge points-to sets
  for (const auto &[KeyPtr, Set] : OtherPTI.AliasSets) {
    bool FoundElemPtr = false;
    for (const auto *ElemPtr : *Set) {
      // check if a pointer of other is already present in this

      if (auto Search = AliasSets.find(ElemPtr); Search != AliasSets.end()) {
        // if so, copy its elements
        FoundElemPtr = true;
        Search->second->insert(Set->begin(), Set->end());
        // and reindex its elements
        for (const auto *ElemPtr : *Set) {
          AliasSets.insert({ElemPtr, Search->second});
        }
        break;
      }
    }
    // if none of the pointers of a set of other is known in this, we need to
    // perform a copy
    if (!FoundElemPtr) {
      auto PTS = Owner.acquire();
      *PTS = *Set;
      AliasSets.try_emplace(KeyPtr, PTS);
    }
  }
}

void LLVMAliasSet::introduceAlias(const llvm::Value *V1, const llvm::Value *V2,
                                  [[maybe_unused]] const llvm::Instruction *I,
                                  [[maybe_unused]] AliasResult Kind) {
  //  only introduce aliases if both values are interesting pointer
  if (!isInterestingPointer(V1) || !isInterestingPointer(V2)) {
    return;
  }
  // before introducing additional aliases make sure we initially computed
  // the aliases for V1 and V2
  computeValuesAliasSet(V1);
  computeValuesAliasSet(V2);
  mergeAliasSets(V1, V2);
}

nlohmann::json LLVMAliasSet::getAsJson() const {
  nlohmann::json J;

  /// Serialize the AliasSets
  auto &Sets = J["AliasSets"];

  for (const AliasSetTy *PTS : Owner.getAllAliasSets()) {
    auto PtsJson = nlohmann::json::array();
    for (const auto *Alias : *PTS) {
      auto Id = getMetaDataID(Alias);
      if (Id != "-1") {
        PtsJson.push_back(std::move(Id));
      }
    }
    if (!PtsJson.empty()) {
      Sets.push_back(std::move(PtsJson));
    }
  }

  /// Serialize the AnalyzedFunctions
  auto &Fns = J["AnalyzedFunctions"];
  for (const auto *F : AnalyzedFunctions) {
    Fns.push_back(F->getName());
  }
  return J;
}

LLVMAliasSetData LLVMAliasSet::getLLVMAliasSetData() const {
  LLVMAliasSetData Data;

  /// Serialize the AliasSets
  for (const AliasSetTy *PTS : Owner.getAllAliasSets()) {

    std::vector<std::string> PtsJson{};
    for (const auto *Alias : *PTS) {
      auto Id = getMetaDataID(Alias);
      if (Id != "-1") {
        PtsJson.push_back(std::move(Id));
      }
    }
    if (!PtsJson.empty()) {
      Data.AliasSets.push_back(std::move(PtsJson));
    }
  }

  /// Serialize the AnalyzedFunctions
  for (const auto *F : AnalyzedFunctions) {
    Data.AnalyzedFunctions.push_back(F->getName().str());
  }

  return Data;
}

void LLVMAliasSet::printAsJson(llvm::raw_ostream &OS) const {
  LLVMAliasSetData Data = getLLVMAliasSetData();
  Data.printAsJson(OS);
}

void LLVMAliasSet::print(llvm::raw_ostream &OS) const {
  for (const auto &[V, PTS] : AliasSets) {
    OS << "V: " << llvmIRToString(V) << '\n';
    for (const auto &Ptr : *PTS) {
      OS << "\tpoints to -> " << llvmIRToString(Ptr) << '\n';
    }
  }
}

void LLVMAliasSet::peakIntoAliasSet(const AliasSetMap::value_type &ValueSetPair,
                                    int Peak) {
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

void LLVMAliasSet::drawAliasSetsDistribution(int Peak) const {
  std::vector<std::pair<size_t, unsigned>> SizeAmountPairs;

  for (const auto &ValueSetPair : AliasSets) {
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
    for (const auto &ValueSetPair : AliasSets) {
      if (ValueSetPair.second->size() == SizeAmountPairs.back().first) {
        llvm::outs() << "Peak into one of the biggest points sets.\n";
        peakIntoAliasSet(ValueSetPair, Peak);
        return;
      }
    }
  }
}

} // namespace psr
