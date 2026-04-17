#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/AliasSetOwner.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAliasAnalysisType.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/ValueCompressor.h"

#include <memory>
#include <type_traits>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class LLVMUnionFindAliasSet;
class LLVMProjectIRDB;

template <>
struct AliasInfoTraits<LLVMUnionFindAliasSet>
    : DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {};

/// Concrete \c IsAliasInfo implementation backed by a union-find alias
/// analysis.
///
/// Constructed once from an \c LLVMProjectIRDB and a call graph; the chosen
/// analysis variant is determined by \c Config.  Alias sets are materialized
/// lazily on first query and cached per \c ValueId in \c AliasSets.
///
/// \note When \c AnalysisLocality::FunctionLocal is selected, alias sets are
///   filtered to variables visible in the function that contains the query
///   instruction.  The per-\c ValueId cache does **not** account for the
///   instruction context, so the first caller's function wins — do not mix
///   queries to local pointers from different functions for the same value in
///   local mode.
class LLVMUnionFindAliasSet
    : public AnalysisPropertiesMixin<LLVMUnionFindAliasSet> {
public:
  using traits_t = AliasInfoTraits<LLVMUnionFindAliasSet>;
  using n_t = traits_t::n_t;
  using v_t = traits_t::v_t;
  using AliasSetTy = traits_t::AliasSetTy;
  using AliasSetPtrTy = traits_t::AliasSetPtrTy;
  using AllocationSiteSetPtrTy = traits_t::AllocationSiteSetPtrTy;

  /// Whether alias sets are reported globally or filtered to the function
  /// containing the query instruction.
  enum class AnalysisLocality : uint8_t {
    /// All aliases across all functions are reported.
    Global,
    /// Aliases are intersected with variables visible in the querying function
    /// (globals + function-local values).
    FunctionLocal,
  };

  struct Config {
    /// The specific union-find analysis variant to run (default:
    /// \c BotCtxIndSens — bottom-up, context- and indirection-sensitive).
    UnionFindAliasAnalysisType AType =
        UnionFindAliasAnalysisType::BotCtxIndSens;
    /// Controls whether alias sets are scoped to the querying function.
    AnalysisLocality ALocality = AnalysisLocality::Global;
  };

  explicit LLVMUnionFindAliasSet(const LLVMProjectIRDB *IRDB,
                                 const LLVMBasedCallGraph &BaseCG, Config Cfg,
                                 ValueCompressor<PAGVariable> *VC);
  explicit LLVMUnionFindAliasSet(const LLVMProjectIRDB *IRDB,
                                 const LLVMBasedCallGraph &BaseCG, Config Cfg)
      : LLVMUnionFindAliasSet(IRDB, BaseCG, Cfg, nullptr) {}
  explicit LLVMUnionFindAliasSet(const LLVMProjectIRDB *IRDB,
                                 const LLVMBasedCallGraph &BaseCG)
      : LLVMUnionFindAliasSet(IRDB, BaseCG, Config{}, nullptr) {}

  [[nodiscard]] constexpr std::true_type isInterProcedural() const noexcept {
    return {};
  };

  [[nodiscard]] constexpr std::integral_constant<AliasAnalysisType,
                                                 AliasAnalysisType::UnionFind>
  getAliasAnalysisType() const noexcept {
    return {};
  };

  [[nodiscard]] constexpr AnalysisProperties
  getAnalysisProperties() const noexcept {
    return Props;
  }

  [[nodiscard]] constexpr AliasResult alias(v_t V1, v_t V2, n_t I) const {
    assert(isValid());
    return AARes->alias(V1, V2, I);
  }

  void foreachAliasOf(v_t V, n_t I,
                      llvm::function_ref<void(v_t)> WithAlias) const {
    assert(isValid());
    AARes->forallAliasesOf(V, I, WithAlias);
  }

  [[nodiscard]] AliasSetPtrTy getAliasSet(v_t V, n_t I) {
    assert(isValid());
    auto ValId = AARes->VC->getOrNull(V);
    if (!ValId) {
      return getEmptyAliasSet();
    }

    assert(AliasSets.inbounds(*ValId));
    if (!AliasSets[*ValId]) [[unlikely]] {
      AliasSets[*ValId] = AARes->constructAliasSet(*ValId, I, Owner);
    }

    return AliasSets[*ValId];
  }

  [[nodiscard]] AllocationSiteSetPtrTy
  getReachableAllocationSites(v_t V, bool IntraProcOnly, n_t I) const {
    assert(isValid());
    auto ValId = AARes->VC->getOrNull(V);
    if (!ValId) {
      return std::make_unique<AliasSetTy>();
    }

    return AARes->constructReachableAllocSites(V, *ValId, IntraProcOnly, I);
  }

  [[nodiscard]] bool isInReachableAllocationSites(
      const llvm::Value *V, const llvm::Value *PotentialValue,
      bool IntraProcOnly, const llvm::Instruction *I) const {
    assert(isValid());
    if (!psr::isInterestingPointer(V)) {
      return false;
    }

    if (!psr::isInReachableAllocationSitesTy(V, PotentialValue,
                                             IntraProcOnly)) {
      return false;
    }

    return alias(V, PotentialValue, I) != AliasResult::NoAlias;
  }

  void print(llvm::raw_ostream &OS) const;
  void printAsJson(llvm::raw_ostream &OS) const;

  [[nodiscard]] bool isValid() const noexcept {
    return AARes != nullptr && Props != AnalysisProperties::None &&
           AARes->VC != nullptr && AARes->VC->size() == AliasSets.size();
  }

private:
  struct UnionFindAAResultConcept {
    MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC;
    std::optional<RawAliasSet<ValueId>> AllocationSites{};

    UnionFindAAResultConcept(
        MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC) noexcept
        : VC(std::move(VC)) {}
    virtual ~UnionFindAAResultConcept() = default;

    virtual void forallAliasesOf(v_t Ptr, n_t Inst,
                                 llvm::function_ref<void(v_t)> Callback) = 0;

    virtual AliasResult alias(v_t Ptr1, v_t Ptr2, n_t AtInstruction) = 0;

    virtual AliasSetPtrTy
    constructAliasSet(ValueId ValId, n_t Inst,
                      AliasSetOwner<AliasSetTy> &Owner) = 0;

    virtual AllocationSiteSetPtrTy
    constructReachableAllocSites(v_t V, ValueId ValId, bool IntraProcOnly,
                                 n_t Inst) = 0;

    virtual void print(llvm::raw_ostream &OS, Config Cfg) const = 0;
  };

  template <template <typename, typename> typename AAResIterT, typename AAResT>
  struct UnionFindAAResultModel;

  [[nodiscard]] static BoxedPtr<AliasSetTy> getEmptyAliasSet();

  // --- data members:

  std::unique_ptr<UnionFindAAResultConcept> AARes{};
  AnalysisProperties Props{};
  Config Cfg{};

  AliasSetOwner<AliasSetTy>::memory_resource_type MRes{};
  AliasSetOwner<AliasSetTy> Owner{&MRes};
  TypedVector<ValueId, AliasSetPtrTy> AliasSets;
};

[[nodiscard]] llvm::StringRef
to_string(LLVMUnionFindAliasSet::AnalysisLocality ALoc) noexcept;
} // namespace psr
