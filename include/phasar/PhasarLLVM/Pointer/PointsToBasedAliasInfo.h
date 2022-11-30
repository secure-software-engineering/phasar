/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOBASEDALIASINFO_H
#define PHASAR_PHASARLLVM_POINTER_POINTSTOBASEDALIASINFO_H

#include "phasar/PhasarLLVM/Pointer/AliasInfoBase.h"
#include "phasar/PhasarLLVM/Pointer/AliasSetOwner.h"
#include "phasar/PhasarLLVM/Pointer/PointsToInfoBase.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/ErrorHandling.h"

#include <utility>

namespace psr {
/// TODO: Implement something generic to make an AliasInfo out of a PointsToInfo
// Idea: For the simple alias-query just check whether the points-to sets
//       overlap
//       For the getAliasSet, we loop over all points-to information for that
//       instruction and check which points-to sets do overlap.
template <typename PTA> class PointsToBasedAliasInfo;

template <typename PTA>
struct AliasInfoTraits<PointsToBasedAliasInfo<PTA>>
    : DefaultAATraits<typename PointsToTraits<PTA>::v_t,
                      typename PointsToTraits<PTA>::n_t> {};

template <typename PTA>
class PointsToBasedAliasInfo : AliasInfoBase<PointsToBasedAliasInfo<PTA>> {
  using base_t = AliasInfoBase<PointsToBasedAliasInfo<PTA>>;

public:
  using typename base_t::AliasSetPtrTy;
  using typename base_t::AliasSetTy;
  using typename base_t::AllocationSiteSetPtrTy;
  using typename base_t::n_t;
  using typename base_t::v_t;
  using o_t = typename PTA::o_t;
  using PointsToSetTy = typename PTA::PointsToSetTy;
  using PointsToSetPtrTy = typename PTA::PointsToSetPtrTy;

  explicit PointsToBasedAliasInfo(const PointsToInfoBase<PTA> *PT) noexcept
      : PT(PT) {
    assert(PT != nullptr);
  }

private:
  // --- Impl for AliasInfoBase
  [[nodiscard]] bool isInterProceduralImpl() const noexcept {
    /// TODO: implement
    return true;
  }
  [[nodiscard]] AliasAnalysisType getAliasAnalysisTypeImpl() const noexcept {
    return AliasAnalysisType::PointsTo;
  }

  [[nodiscard]] bool overlap(ByConstRef<PointsToSetPtrTy> PT1,
                             ByConstRef<PointsToSetPtrTy> PT2) {
    if (PT1->size() > PT2->size()) {
      std::swap(PT1, PT2);
    }
    for (const auto &Obj : PT1) {
      if (PT2->count(Obj)) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] AliasResult
  aliasImpl(ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
            ByConstRef<n_t> AtInstruction = {}) const {
    /// XXX: Make use of LLVM's TBAA information to increase precision

    auto PT1 = PT->getPointsToSet(Pointer1, AtInstruction);
    auto PT2 = PT->getPointsToSet(Pointer2, AtInstruction);
    return overlap(PT1, PT2) ? AliasResult::MayAlias : AliasResult::NoAlias;
  }

  [[nodiscard]] AliasSetPtrTy
  getAliasSetImpl(ByConstRef<v_t> Pointer, ByConstRef<n_t> AtInstruction = {}) {
    if (auto It = Cache.find({AtInstruction, Pointer}); It != Cache.end()) {
      return It->second;
    }

    auto PTS = PT->getPointsToSet(Pointer, AtInstruction);
    auto Ret = Owner.acquire();
    Ret->insert(Pointer);

    for (ByConstRef<v_t> Ptr : PT->getInterestingPointersAt(AtInstruction)) {
      if (Ptr == Pointer) {
        continue;
      }

      /// XXX: Make use of LLVM's TBAA information to increase precision

      auto PtrPTS = PT->getPointsToSet(Ptr, AtInstruction);

      if (overlap(PTS, PtrPTS)) {
        Ret->insert(Ptr);
      }
    }

    if (Ret->size() == 1) {
      auto &Singleton = SingletonSets[Pointer];
      if (!Singleton) {
        Singleton = Ret;
      } else {
        Owner.release(Ret);
      }
      Cache.try_emplace(std::make_pair(AtInstruction, Pointer), Singleton);
      return Singleton;
    }

    // Note: Alias information in general is not transitive; for simplicity, we
    // assume transitivity here to deal with it as an equivalence relation.

    for (ByConstRef<v_t> Ptr : *Ret) {
      assert(!Cache.count({AtInstruction, Ptr}) &&
             "Assume transitivity of the alias-relation");

      Cache.try_emplace(std::make_pair(AtInstruction, Ptr), Ret);
    }

    return Ret;
  }

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSitesImpl(ByConstRef<v_t> Pointer,
                                  bool IntraProcOnly = false,
                                  ByConstRef<n_t> AtInstruction = {}) const {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }

  // Checks if Pointer2 is a reachable allocation in the alias set of
  // Pointer1.
  [[nodiscard]] bool isInReachableAllocationSitesImpl(
      ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
      bool IntraProcOnly = false, ByConstRef<n_t> AtInstruction = {}) const {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }

  void printImpl(llvm::raw_ostream &OS = llvm::outs()) const {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }

  [[nodiscard]] nlohmann::json getAsJsonImpl() const {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }

  void printAsJsonImpl(llvm::raw_ostream &OS) const {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }

  template <typename AI,
            typename = std::enable_if_t<std::is_same_v<typename AI::n_t, n_t> &&
                                        std::is_same_v<typename AI::v_t, v_t>>>
  void mergeWithImpl(const AliasInfoBase<AI> &Other) {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }

  void introduceAliasImpl(ByConstRef<v_t> Pointer1, ByConstRef<v_t> Pointer2,
                          ByConstRef<n_t> AtInstruction = {},
                          AliasResult Kind = AliasResult::MustAlias) {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }

  [[nodiscard]] AnalysisProperties getAnalysisPropertiesImpl() const noexcept {
    /// TODO: implement
    llvm_unreachable("Not implemented");
  }
  // ---

  const PointsToInfoBase<PTA> *PT{};
  AliasSetOwner<AliasSetTy> Owner;
  llvm::DenseMap<v_t, DynamicAliasSetPtr<AliasSetTy>> SingletonSets;
  // XXX: Optimize this cache structure! Maybe query for flow-sensitivity?
  llvm::DenseMap<std::pair<n_t, v_t>, DynamicAliasSetPtr<AliasSetTy>> Cache;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_POINTSTOBASEDALIASINFO_H
