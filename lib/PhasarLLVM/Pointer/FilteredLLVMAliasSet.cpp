#include "phasar/PhasarLLVM/Pointer/FilteredLLVMAliasSet.h"

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Instructions.h"

#include "nlohmann/json_fwd.hpp"

#include <memory>
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

[[nodiscard]] static bool
isConstantGlobalValue(const llvm::GlobalValue *GlobV) {
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
    if (llvm::isa<llvm::AllocaInst>(p2) || isConstantGlobalValue(Glob1)) {
      return true;
    }
    if (const auto *Glob2 = llvm::dyn_cast<llvm::GlobalValue>(p2)) {
      return true; // approximation
    }
  } else if (const auto *Glob2 = llvm::dyn_cast<llvm::GlobalValue>(p2)) {
    return isConstantGlobalValue(Glob2);
  }

  return false;
}

template <typename WithAliasFn>
static void foreachValidAliasIn(LLVMAliasSet::AliasSetPtrTy AS,
                                const llvm::Value *V, const llvm::Function *Fun,
                                WithAliasFn &&WithAlias) {
  if (!Fun) {
    llvm::for_each(*AS, WithAlias);
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
      std::invoke(WithAlias, Alias);
      continue;
    }

    if (llvm::isa<llvm::ConstantExpr, llvm::ConstantData>(Alias)) {
      // Assume: Compile-time constants are not generated as data-flow facts!
      continue;
    }

    const auto *AliasBase = Alias->stripPointerCastsAndAliases();

    if (mustNoalias(Base, AliasBase)) {
      continue;
    }

    std::invoke(WithAlias, Alias);
  }
}

FilteredLLVMAliasSet::FilteredLLVMAliasSet(LLVMAliasSet *AS) noexcept
    : AS(AS), Owner(&AS->MRes) {}

FilteredLLVMAliasSet::FilteredLLVMAliasSet(
    MaybeUniquePtr<LLVMAliasSet, true> AS) noexcept
    : AS(std::move(AS)), Owner(&this->AS->MRes) {}

FilteredLLVMAliasSet::~FilteredLLVMAliasSet() = default;

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
  if (!isInterestingPointer(V)) {
    return AS->getEmptyAliasSet();
  }

  auto &Entry = AliasSetMap[{Fun, V}];
  if (!Entry) {
    auto Set = Owner.acquire();
    foreachValidAliasIn(AS->getAliasSet(V), V, Fun,
                        [&Set](v_t Alias) { Set->insert(Alias); });
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

  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return &getDefaultValue<AliasSetTy>();
  }

  const auto *Fun = I->getFunction();
  auto &AllocSites = ReachableAllocationSitesMap[ReachableAllocationSitesKey{
      {Fun, IntraProcOnly}, V}];
  if (AllocSites) {
    return AllocSites.get();
  }

  AllocSites = std::make_unique<AliasSetTy>();

  // consider the full inter-procedural points-to/alias information
  if (!IntraProcOnly) {
    foreachValidAliasIn(AS->getAliasSet(V), V, Fun,
                        [Set = AllocSites.get(), AS = AS.get(), V](v_t Alias) {
                          if (AS->interIsReachableAllocationSiteTy(V, Alias)) {
                            Set->insert(Alias);
                          }
                        });

  } else {
    // consider the function-local, i.e. intra-procedural, points-to/alias
    // information only

    // We may not be able to retrieve a function for the given value since some
    // pointer values can exist outside functions, for instance, in case of
    // vtables, etc.
    const auto *VFun = getFunction(V);
    const auto *VG = llvm::dyn_cast<llvm::GlobalObject>(V);
    foreachValidAliasIn(
        AS->getAliasSet(V), V, Fun,
        [Set = AllocSites.get(), AS = AS.get(), V, VFun, VG](v_t Alias) {
          if (AS->intraIsReachableAllocationSiteTy(V, Alias, VFun, VG)) {
            Set->insert(Alias);
          }
        });
  }
  return AllocSites.get();
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
