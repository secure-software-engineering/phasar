/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_CACHEDLLVMALIASITERATOR_H
#define PHASAR_PHASARLLVM_POINTER_CACHEDLLVMALIASITERATOR_H

#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Pointer/AliasSetOwner.h"

#include "llvm/IR/Function.h"

namespace psr {

class CachedLLVMAliasIterator;

template <>
struct AliasInfoTraits<CachedLLVMAliasIterator>
    : DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {};

/// \brief A wrapper over a LLVMAliasIteratorRef that adds caching to make it
/// conform to the LLVMAliasInfoRef interface
///
/// \note Currently assumes that the underlying alias information is
/// flow-insensitive and the granularity of different alias-information per
/// instruction is actually at function-level
class CachedLLVMAliasIterator {
public:
  using alias_traits_t = AliasInfoTraits<CachedLLVMAliasIterator>;
  using n_t = alias_traits_t::n_t;
  using v_t = alias_traits_t::v_t;
  using AliasSetTy = alias_traits_t::AliasSetTy;
  using AliasSetPtrTy = alias_traits_t::AliasSetPtrTy;
  using AllocationSiteSetPtrTy = alias_traits_t::AllocationSiteSetPtrTy;

  CachedLLVMAliasIterator(LLVMAliasIteratorRef AS) noexcept;

  // --- API Functions:

  [[nodiscard]] inline bool isInterProcedural() const noexcept {
    return false; // No idea, so be conservative here
  };

  [[nodiscard]] AliasAnalysisType getAliasAnalysisType() const noexcept {
    return AliasAnalysisType::Invalid; // No idea
  }

  [[nodiscard]] AliasResult alias(const llvm::Value *V1, const llvm::Value *V2,
                                  const llvm::Instruction *I);

  [[nodiscard]] AliasSetPtrTy getAliasSet(const llvm::Value *V,
                                          const llvm::Instruction *I);

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSites(const llvm::Value *V, bool IntraProcOnly = false,
                              const llvm::Instruction *I = nullptr);

  // Checks if PotentialValue is in the reachable allocation sites of V.
  [[nodiscard]] bool isInReachableAllocationSites(
      const llvm::Value *V, const llvm::Value *PotentialValue,
      bool IntraProcOnly = false, const llvm::Instruction *I = nullptr);

  void mergeWith(const CachedLLVMAliasIterator & /*OtherPTI*/) {
    llvm::report_fatal_error("Not Supported");
  }

  void introduceAlias(const llvm::Value * /*V1*/, const llvm::Value * /*V2*/,
                      const llvm::Instruction * /*I*/ = nullptr,
                      AliasResult /*Kind*/ = AliasResult::MustAlias) {
    llvm::report_fatal_error("Not Supported");
  }

  void print(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] nlohmann::json getAsJson() const;

  void printAsJson(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] AnalysisProperties getAnalysisProperties() const noexcept {
    return AnalysisProperties::None;
  }

  [[nodiscard]] LLVMAliasIteratorRef getUnderlying() const noexcept {
    return AS;
  }

private:
  struct ReachableAllocationSitesKey {
    llvm::PointerIntPair<const llvm::Function *, 1, bool> FunAndIntraProcOnly;
    v_t Value{};
  };

  struct ReachableAllocationSitesKeyDMI {
    inline static ReachableAllocationSitesKey getEmptyKey() noexcept {
      return {{}, llvm::DenseMapInfo<v_t>::getEmptyKey()};
    }
    inline static ReachableAllocationSitesKey getTombstoneKey() noexcept {
      return {{}, llvm::DenseMapInfo<v_t>::getTombstoneKey()};
    }
    inline static auto getHashValue(ReachableAllocationSitesKey Key) noexcept {
      return llvm::hash_combine(Key.FunAndIntraProcOnly.getOpaqueValue(),
                                Key.Value);
    }
    inline static bool isEqual(ReachableAllocationSitesKey Key1,
                               ReachableAllocationSitesKey Key2) noexcept {
      return Key1.FunAndIntraProcOnly == Key2.FunAndIntraProcOnly &&
             Key1.Value == Key2.Value;
    }
  };

  LLVMAliasIteratorRef AS;
  AliasSetOwner<AliasSetTy>::memory_resource_type MRes;
  AliasSetOwner<AliasSetTy> Owner;
  llvm::DenseMap<std::pair<const llvm::Function *, v_t>, AliasSetPtrTy>
      AliasSetMap;
  llvm::DenseMap<ReachableAllocationSitesKey, std::unique_ptr<AliasSetTy>,
                 ReachableAllocationSitesKeyDMI>
      ReachableAllocationSitesMap;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_CACHEDALIASITERATOR_H
