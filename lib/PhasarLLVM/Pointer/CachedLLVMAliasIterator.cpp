#include "phasar/PhasarLLVM/Pointer/CachedLLVMAliasIterator.h"

#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/NlohmannLogging.h"

#include "FilteredAliasesUtils.h"

using namespace psr;

CachedLLVMAliasIterator::CachedLLVMAliasIterator(
    LLVMAliasIteratorRef AS) noexcept
    : AS(AS), Owner(&MRes) {}

AliasResult CachedLLVMAliasIterator::alias(const llvm::Value *V1,
                                           const llvm::Value *V2,
                                           const llvm::Instruction *I) {
  auto V1AS = getAliasSet(V1, I);
  return V1AS->contains(V2) ? AliasResult::MayAlias : AliasResult::NoAlias;
}

static BoxedPtr<CachedLLVMAliasIterator::AliasSetTy> getEmptyAliasSet() {
  static CachedLLVMAliasIterator::AliasSetTy EmptySet{};
  static CachedLLVMAliasIterator::AliasSetTy *EmptySetPtr = &EmptySet;
  return &EmptySetPtr;
}

auto CachedLLVMAliasIterator::getAliasSet(const llvm::Value *V,
                                          const llvm::Instruction *I)
    -> AliasSetPtrTy {
  if (!isInterestingPointer(V)) {
    return getEmptyAliasSet();
  }

  const auto *Fun = I ? I->getFunction() : nullptr;

  // XXX: This may be wrong, if the underlying alias analysis is more precise
  // than on function-level!
  // Checkout the comment on the CachedLLVMAliasIterator for that; should be
  // fine for now.
  auto &Entry = AliasSetMap[{Fun, V}];
  if (!Entry) {
    auto Set = Owner.acquire();
    AS.forallAliasesOf(V, I, [&Set](v_t Alias) { Set->insert(Alias); });
    Entry = Set;
  }
  return Entry;
}

auto CachedLLVMAliasIterator::getReachableAllocationSites(
    const llvm::Value *V, bool IntraProcOnly, const llvm::Instruction *I)
    -> AllocationSiteSetPtrTy {

  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return &getDefaultValue<AliasSetTy>();
  }

  const auto *Fun = I ? I->getFunction() : nullptr;
  auto &AllocSites = ReachableAllocationSitesMap[ReachableAllocationSitesKey{
      {Fun, IntraProcOnly}, V}];
  if (AllocSites) {
    return AllocSites.get();
  }

  AllocSites = std::make_unique<AliasSetTy>();

  const auto *VFun = getFunction(V);
  const auto *VG = llvm::dyn_cast<llvm::GlobalObject>(V);

  AS.forallAliasesOf(
      V, I, [Set = AllocSites.get(), V, IntraProcOnly, VFun, VG](v_t Alias) {
        if (psr::isInReachableAllocationSitesTy(V, Alias, IntraProcOnly, VFun,
                                                VG)) {
          Set->insert(Alias);
        }
      });

  return AllocSites.get();
}

// Checks if PotentialValue is in the reachable allocation sites of V.
bool CachedLLVMAliasIterator::isInReachableAllocationSites(
    const llvm::Value *V, const llvm::Value *PotentialValue, bool IntraProcOnly,
    const llvm::Instruction *I) {

  if (PotentialValue == V) {
    return true;
  }

  // if V is not a (interesting) pointer we can return an empty set
  if (!isInterestingPointer(V)) {
    return false;
  }

  bool PVIsReachableAllocationSiteType =
      psr::isInReachableAllocationSitesTy(V, PotentialValue, IntraProcOnly);

  if (PVIsReachableAllocationSiteType) {
    const auto PTS = getAliasSet(V, I);
    return PTS->count(PotentialValue);
  }

  return false;
}

void CachedLLVMAliasIterator::print(llvm::raw_ostream &OS) const {
  for (const auto &[FV, PTS] : AliasSetMap) {
    OS << "V: " << llvmIRToString(FV.second) << " in function '"
       << FV.first->getName() << "'\n";
    for (const auto &Ptr : *PTS) {
      OS << "\taliases with -> " << llvmIRToString(Ptr) << '\n';
    }
  }
}

nlohmann::json CachedLLVMAliasIterator::getAsJson() const {
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

void CachedLLVMAliasIterator::printAsJson(llvm::raw_ostream &OS) const {
  OS << getAsJson();
}

static_assert(
    std::is_convertible_v<CachedLLVMAliasIterator *, LLVMAliasInfoRef>);
