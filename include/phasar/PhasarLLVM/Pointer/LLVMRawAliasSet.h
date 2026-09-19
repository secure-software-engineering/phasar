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
#include "phasar/PhasarLLVM/Pointer/LLVMRawAAResults.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/AliasSetOwner.h"
#include "phasar/Pointer/RawAAResult.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAliasAnalysisType.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/IR/Instructions.h"
#include "llvm/Support/TypeName.h"

#include <memory>
#include <type_traits>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class LLVMRawAliasSet;
class LLVMProjectIRDB;

template <>
struct AliasInfoTraits<LLVMRawAliasSet>
    : DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {};

struct LLVMRawAliasSetBase {
  using traits_t = AliasInfoTraits<LLVMRawAliasSet>;
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
};

[[nodiscard]] llvm::StringRef
to_string(LLVMRawAliasSetBase::AnalysisLocality ALoc) noexcept;

/// Concrete \c IsAliasInfo implementation backed by a raw alias
/// analysis. Provides convenience constructors to invoke union-find-based
/// analyses, but can be instantiated with *any* analysis result that conforms
/// to \c RawAAResult .
///
/// Alias sets are materialized lazily on first query and cached per \c ValueId
/// in \c AliasSets.
///
/// \note When \c AnalysisLocality::FunctionLocal is selected, alias sets are
///   filtered to variables visible in the function that contains the query
///   instruction.  The per-\c ValueId cache does **not** account for the
///   instruction context, so the first caller's function wins — do not mix
///   queries to local pointers from different functions for the same value in
///   local mode.
class LLVMRawAliasSet : public LLVMRawAliasSetBase,
                        public AnalysisPropertiesMixin<LLVMRawAliasSet> {
public:
  explicit LLVMRawAliasSet(const LLVMProjectIRDB *IRDB,
                           const LLVMBasedCallGraph &BaseCG, Config Cfg,
                           ValueCompressor<PAGVariable> *VC);
  explicit LLVMRawAliasSet(const LLVMProjectIRDB *IRDB,
                           const LLVMBasedCallGraph &BaseCG, Config Cfg)
      : LLVMRawAliasSet(IRDB, BaseCG, Cfg, nullptr) {}
  explicit LLVMRawAliasSet(const LLVMProjectIRDB *IRDB,
                           const LLVMBasedCallGraph &BaseCG)
      : LLVMRawAliasSet(IRDB, BaseCG, Config{}, nullptr) {}

  template <RawAAResult AAResT>
  explicit LLVMRawAliasSet(AAResT &&AARes,
                           MaybeUniquePtr<ValueCompressor<PAGVariable>> VC,
                           AnalysisProperties Props = {})
      : Props(Props) {
    assert(VC != nullptr);
    AliasSets.resize(VC->size());
    // XXX: Support locality

    this->AARes = std::make_unique<
        AAResultModel<LLVMRawAliasIteratorMixin, std::remove_cvref_t<AAResT>>>(
        std::move(VC), PSR_FWD(AARes));
  }

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
    return AARes != nullptr && AARes->VC != nullptr &&
           AARes->VC->size() == AliasSets.size();
  }

private:
  struct AAResultConcept {
    MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC;
    std::optional<RawAliasSet<ValueId>> AllocationSites{};

    AAResultConcept(
        MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC) noexcept
        : VC(std::move(VC)) {}
    virtual ~AAResultConcept() = default;

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

  static bool isPotentialAllocSite(const llvm::Value *Val) {
    if (!Val->getType()->isPointerTy()) {
      return false;
    }
    if (llvm::isa<llvm::AllocaInst, llvm::Argument>(Val)) {
      return true;
    }
    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Val)) {
      return Call->getCalledFunction() &&
             psr::isHeapAllocatingFunction(Call->getCalledFunction());
    }
    return false;
  }

  template <template <typename, typename> typename AAResIterT, typename AAResT>
  struct AAResultModel
      : public AAResultConcept,
        public AAResIterT<AAResultModel<AAResIterT, AAResT>, AAResT> {

    using base_t = AAResIterT<AAResultModel<AAResIterT, AAResT>, AAResT>;

    template <typename... ArgsT>
    constexpr AAResultModel(
        MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC, ArgsT &&...Args)
        : AAResultConcept{std::move(VC)}, base_t{PSR_FWD(Args)...} {}

    void forallAliasesOf(v_t Ptr, n_t Inst,
                         llvm::function_ref<void(v_t)> Callback) override {
      this->base_t::forallAliasesOf(Ptr, Inst, Callback);
    }

    AliasResult alias(v_t Ptr1, v_t Ptr2,
                      n_t AtInstruction = nullptr) override {
      return this->base_t::alias(Ptr1, Ptr2, AtInstruction);
    }

    AliasSetPtrTy constructAliasSet(ValueId ValId, n_t Inst,
                                    AliasSetOwner<AliasSetTy> &Owner) override {
      auto ASet = Owner.acquire();
      try {
        this->base_t::forallAliasesOf(
            ValId, Inst,
            [ASet{ASet.get()}](v_t Alias) -> void { ASet->insert(Alias); });
        return ASet;
      } catch (...) {
        Owner.release(ASet.get());
        throw;
      }
    }

    AllocationSiteSetPtrTy constructReachableAllocSites(v_t V, ValueId ValId,
                                                        bool IntraProcOnly,
                                                        n_t Inst) override {
      if (!this->AllocationSites) [[unlikely]] {
        this->AllocationSites.emplace();
        for (const auto &[VId, Vars] : this->VC->id2vars().enumerate()) {
          for (const PAGVariable &Var : Vars) {
            if (const auto *LLVMVar = Var.valueOrNull();
                LLVMVar && isPotentialAllocSite(LLVMVar)) {
              this->AllocationSites->insert(VId);
              break;
            }
          }
        }
      }

      auto RawAliases = [&]() -> RawAliasSet<ValueId> {
        if constexpr (requires() {
                        this->base_t::getRawAliasSet(ValId, Inst);
                      }) {
          return this->base_t::getRawAliasSet(ValId, Inst);
        } else {
          return this->base_t::getRawAliasSet(ValId);
        }
      }();
      RawAliases &= *this->AllocationSites;

      auto Ret = std::make_unique<AliasSetTy>();

      RawAliases.foreach (
          llvmRawAliasHandler(*this->VC, [&Ret, V, IntraProcOnly](v_t Alias) {
            if (psr::isInReachableAllocationSitesTy(V, Alias, IntraProcOnly)) {
              Ret->insert(Alias);
            }
          }));

      return Ret;
    }

    void print(llvm::raw_ostream &OS, Config Cfg) const override {
      OS << "LLVMRawAliasSet(" << to_string(Cfg.ALocality) << ", "
         << llvm::getTypeName<AAResT>() << ") {\n";

      for (auto ValId : iota<ValueId>(VC->size())) {
        OS << "  #" << psr::to_underlying(ValId) << ": {";
        bool First = true;
        const auto &Aliases = this->base_t::getRawAliasSet(ValId);
        Aliases.foreach ([&](auto AliasId) {
          if (First) {
            First = false;
          } else {
            OS << ", ";
          }

          OS << psr::to_underlying(AliasId);
        });
        OS << "}\n";
      }
      OS << "}\n";
    };
  };

  [[nodiscard]] static BoxedPtr<AliasSetTy> getEmptyAliasSet();

  // --- data members:

  std::unique_ptr<AAResultConcept> AARes{};
  AnalysisProperties Props{};
  Config Cfg{};

  AliasSetOwner<AliasSetTy>::memory_resource_type MRes{};
  AliasSetOwner<AliasSetTy> Owner{&MRes};
  TypedVector<ValueId, AliasSetPtrTy> AliasSets;
};

} // namespace psr
