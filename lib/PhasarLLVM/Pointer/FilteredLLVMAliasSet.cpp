#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/IR/Instructions.h"

#include "nlohmann/json_fwd.hpp"

#include <type_traits>

using namespace psr;

static const llvm::Function *getFunction(const llvm::Value *V) {
  if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
    return Inst->getFunction();
  }
  if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(V)) {
    return Arg->getParent();
  }
  return nullptr;
}

[[nodiscard]] static bool isConstantGlob(const llvm::GlobalValue *GlobV) {
  if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(GlobV)) {
    return Glob->isConstant();
  }
  if (const auto *Alias = llvm::dyn_cast<llvm::GlobalAlias>(GlobV)) {
    if (const auto *AliasGlob =
            llvm::dyn_cast<llvm::GlobalVariable>(Alias->getAliasee())) {
      return AliasGlob->isConstant();
    }
  }
  return true;
}

static bool mustNoalias(const llvm::Value *p1, const llvm::Value *p2) {
  if (p1 == p2) {
    return false;
  }
  assert(p1);
  assert(p2);

  // Assumptions:
  // - Globals do not alias with allocas
  // - Globals do not alias with each other (this may be a bit unsound, though)
  // - Allocas do not alias each other (relax a bit for allocas of pointers)
  // - Constant globals are not generated as data-flow facts

  if (const auto *Alloca1 = llvm::dyn_cast<llvm::AllocaInst>(p1)) {
    if (llvm::isa<llvm::GlobalValue>(p2)) {
      return true;
    }
    if (const auto *Alloca2 = llvm::dyn_cast<llvm::AllocaInst>(p2)) {
      return !Alloca1->getAllocatedType()->isPointerTy() &&
             !Alloca2->getAllocatedType()->isPointerTy();
    }
  } else if (const auto *Glob1 = llvm::dyn_cast<llvm::GlobalValue>(p1)) {
    if (llvm::isa<llvm::AllocaInst>(p2) || isConstantGlob(Glob1)) {
      return true;
    }
    if (const auto *Glob2 = llvm::dyn_cast<llvm::GlobalValue>(p2)) {
      return true; // approximation
    }
  } else if (const auto *Glob2 = llvm::dyn_cast<llvm::GlobalValue>(p2)) {
    return isConstantGlob(Glob2);
  }

  return false;
}

static void fillAliasSet(FilteredLLVMAliasSet::AliasSetTy &Set,
                         LLVMAliasSet::AliasSetPtrTy AS, const llvm::Value *V,
                         const llvm::Function *Fun) {
  if (!Fun) {
    Set.insert(AS->begin(), AS->end());
    return;
  }

  const auto *Base = V->stripPointerCastsAndAliases();
  for (const auto *Alias : *AS) {

    // Skip inter-procedural aliases
    const auto *AliasFun = getFunction(Alias);
    if (AliasFun && Fun != AliasFun) {
      continue;
    }

    if (V == Alias) {
      Set.insert(Alias);
      continue;
    }

    if (llvm::isa<llvm::ConstantExpr>(Alias) ||
        llvm::isa<llvm::ConstantData>(Alias)) {
      // Assume: Compile-time constants are not generated as data-flow facts!
      continue;
    }

    const auto *AliasBase = Alias->stripPointerCastsAndAliases();

    if (mustNoalias(Base, AliasBase)) {
      continue;
    }

    Set.insert(Alias);
  }
}

FilteredLLVMAliasSet::FilteredLLVMAliasSet(LLVMAliasSet *AS) noexcept
    : AS(AS) {}

FilteredLLVMAliasSet::FilteredLLVMAliasSet(
    MaybeUniquePtr<LLVMAliasSet, true> AS) noexcept
    : AS(std::move(AS)) {}

AliasAnalysisType FilteredLLVMAliasSet::getAliasAnalysisType() const noexcept {
  return AS->getAliasAnalysisType();
}

AliasResult FilteredLLVMAliasSet::alias(const llvm::Value *V1,
                                        const llvm::Value *V2,
                                        const llvm::Function *Fun) {
  auto V1AS = getAliasSet(V1, Fun);
  return V1AS->contains(V2) ? AliasResult::MayAlias : AliasResult::NoAlias;
}

AliasResult FilteredLLVMAliasSet::alias(const llvm::Value *V1,
                                        const llvm::Value *V2,
                                        const llvm::Instruction *I) {
  if (!I) {
    return AS->alias(V1, V2);
  }

  return alias(V1, V2, I->getFunction());
}

auto FilteredLLVMAliasSet::getAliasSet(const llvm::Value *V,
                                       const llvm::Function *Fun)
    -> AliasSetPtrTy {
  auto &Entry = AliasSetMap[{Fun, V}];
  if (!Entry) {
    auto Set = Owner.acquire();
    fillAliasSet(*Set, AS->getAliasSet(V), V, Fun);
    Entry = Set;
  }
  return Entry;
}

auto FilteredLLVMAliasSet::getAliasSet(const llvm::Value *V,
                                       const llvm::Instruction *I)
    -> AliasSetPtrTy {
  const auto *Fun = I ? I->getFunction() : nullptr;
  return getAliasSet(V, Fun);
}

auto FilteredLLVMAliasSet::getReachableAllocationSites(
    const llvm::Value *V, bool IntraProcOnly, const llvm::Instruction *I)
    -> AllocationSiteSetPtrTy {
  auto AllocSites = std::make_unique<AliasSetTy>();

  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return AllocSites;
  }

  const auto PTS = getAliasSet(V, I);
  // consider the full inter-procedural points-to/alias information
  if (!IntraProcOnly) {
    for (const auto *P : *PTS) {
      if (AS->interIsReachableAllocationSiteTy(V, P)) {
        AllocSites->insert(P);
      }
    }
  } else {
    // consider the function-local, i.e. intra-procedural, points-to/alias
    // information only
    const auto *VFun = getFunction(V);
    const auto *VG = llvm::dyn_cast<llvm::GlobalObject>(V);
    // We may not be able to retrieve a function for the given value since some
    // pointer values can exist outside functions, for instance, in case of
    // vtables, etc.
    for (const auto *P : *PTS) {
      if (AS->intraIsReachableAllocationSiteTy(V, P, VFun, VG)) {
        AllocSites->insert(P);
      }
    }
  }
  return AllocSites;
}

// Checks if PotentialValue is in the reachable allocation sites of V.
bool FilteredLLVMAliasSet::isInReachableAllocationSites(
    const llvm::Value *V, const llvm::Value *PotentialValue, bool IntraProcOnly,
    const llvm::Instruction *I) {
  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return false;
  }

  bool PVIsReachableAllocationSiteType = false;
  if (IntraProcOnly) {
    const auto *VFun = getFunction(V);
    const auto *VG = llvm::dyn_cast<llvm::GlobalObject>(V);
    PVIsReachableAllocationSiteType =
        AS->intraIsReachableAllocationSiteTy(V, PotentialValue, VFun, VG);
  } else {
    PVIsReachableAllocationSiteType =
        AS->interIsReachableAllocationSiteTy(V, PotentialValue);
  }

  if (PVIsReachableAllocationSiteType) {
    const auto PTS = getAliasSet(V, I);
    return PTS->count(PotentialValue);
  }

  return false;
}

void FilteredLLVMAliasSet::print(llvm::raw_ostream &OS) const {
  for (const auto &[FV, PTS] : AliasSetMap) {
    OS << "V: " << llvmIRToString(FV.second) << " in function '"
       << FV.first->getName() << "'\n";
    for (const auto &Ptr : *PTS) {
      OS << "\taliases with -> " << llvmIRToString(Ptr) << '\n';
    }
  }
}

nlohmann::json FilteredLLVMAliasSet::getAsJson() const {
  nlohmann::json J;

  for (const auto &[FV, PTS] : AliasSetMap) {
    auto &JFV = J.emplace_back();

    JFV["Value"] = getMetaDataID(FV.second);
    JFV["Function"] = FV.first->getName().str();

    auto &JSet = JFV["Aliases"];

    for (const auto &Ptr : *PTS) {
      JSet.push_back(getMetaDataID(Ptr));
    }
  }

  return J;
}

void FilteredLLVMAliasSet::printAsJson(llvm::raw_ostream &OS) const {
  OS << getAsJson();
}

static_assert(std::is_convertible_v<FilteredLLVMAliasSet *, LLVMAliasInfoRef>);
