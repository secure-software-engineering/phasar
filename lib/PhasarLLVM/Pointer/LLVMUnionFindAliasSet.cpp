#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAliasSet.h"

#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasInfoBase.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/Fn.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/IR/Argument.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include <memory>

using namespace psr;

static_assert(IsAliasInfo<LLVMUnionFindAliasSet>);

static inline bool isPotentialAllocSite(const llvm::Value *Val) {
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
struct [[clang::internal_linkage]] LLVMUnionFindAliasSet::UnionFindAAResultModel
    : public UnionFindAAResultConcept,
      public AAResIterT<UnionFindAAResultModel<AAResIterT, AAResT>, AAResT> {

  using base_t = AAResIterT<UnionFindAAResultModel<AAResIterT, AAResT>, AAResT>;

  template <typename... ArgsT>
  constexpr UnionFindAAResultModel(
      MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC, ArgsT &&...Args)
      : UnionFindAAResultConcept{std::move(VC)}, base_t{PSR_FWD(Args)...} {}

  void forallAliasesOf(v_t Ptr, n_t Inst,
                       llvm::function_ref<void(v_t)> Callback) override {
    this->base_t::forallAliasesOf(Ptr, Inst, Callback);
  }

  AliasResult alias(v_t Ptr1, v_t Ptr2, n_t AtInstruction = nullptr) override {
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
      if constexpr (requires() { this->base_t::getRawAliasSet(ValId, Inst); }) {
        return this->base_t::getRawAliasSet(ValId, Inst);
      } else {
        return this->base_t::getRawAliasSet(ValId);
      }
    }();
    RawAliases &= *this->AllocationSites;

    auto Ret = std::make_unique<AliasSetTy>();

    RawAliases.foreach (llvmUnionFindAliasHandler(
        *this->VC, [&Ret, V, IntraProcOnly](v_t Alias) {
          if (psr::isInReachableAllocationSitesTy(V, Alias, IntraProcOnly)) {
            Ret->insert(Alias);
          }
        }));

    return Ret;
  }

  void print(llvm::raw_ostream &OS, Config Cfg) const override {
    OS << "LLVMUnionFindAliasSet(" << to_string(Cfg.ALocality) << ", "
       << to_string(Cfg.AType) << ") {\n";

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

LLVMUnionFindAliasSet::LLVMUnionFindAliasSet(const LLVMProjectIRDB *IRDB,
                                             const LLVMBasedCallGraph &BaseCG,
                                             Config Cfg,
                                             ValueCompressor<PAGVariable> *VC)
    : Cfg(Cfg) {
  MaybeUniquePtr<ValueCompressor<PAGVariable>> VCOwn = VC;
  if (!VC) {
    VCOwn = std::make_unique<ValueCompressor<PAGVariable>>();
  }

  auto MakeAAResModel =
      [&, IRDB, Cfg, VCOwn = std::move(VCOwn)](
          auto AAResCtor) mutable -> std::unique_ptr<UnionFindAAResultConcept> {
    auto AARes = AAResCtor(*IRDB, BaseCG, VCOwn.get());
    using AAResT = decltype(AARes);

    if (Cfg.ALocality == AnalysisLocality::FunctionLocal) {
      const auto &VCRef = *VCOwn;
      return std::make_unique<
          UnionFindAAResultModel<LLVMLocalUnionFindAliasIteratorMixin, AAResT>>(
          std::move(VCOwn), PSR_FWD(AARes), VCRef);
    }

    return std::make_unique<
        UnionFindAAResultModel<LLVMUnionFindAliasIteratorMixin, AAResT>>(
        std::move(VCOwn), PSR_FWD(AARes));
  };

  scope_exit ResizeAliasSetCache = [&] {
    if (!AARes) {
      // Something went wrong and we hopefully have an in-flight
      // exception...
      return;
    }

    AliasSets.resize(AARes->VC->size());
  };

  switch (Cfg.AType) {
  case UnionFindAliasAnalysisType::CtxSens:
    Props = AnalysisProperties::ContextSensitive;
    AARes = MakeAAResModel(fn<computeCtxSensUnionFindAARaw>);
    return;
  case UnionFindAliasAnalysisType::IndSens:
    Props = AnalysisProperties::FieldSensitive;
    AARes = MakeAAResModel(fn<computeIndSensUnionFindAARaw>);
    return;
  case UnionFindAliasAnalysisType::CtxIndSens:
    Props = AnalysisProperties::ContextSensitive |
            AnalysisProperties::FieldSensitive;
    AARes = MakeAAResModel(fn<computeCtxIndSensUnionFindAARaw>);
    return;
  case UnionFindAliasAnalysisType::BotCtxSens:
    Props = AnalysisProperties::ContextSensitive;
    AARes = MakeAAResModel(fn<computeBotCtxSensUnionFindAARaw>);
    return;
  case UnionFindAliasAnalysisType::BotCtxIndSens:
    Props = AnalysisProperties::ContextSensitive |
            AnalysisProperties::FieldSensitive;
    AARes = MakeAAResModel(fn<computeBotCtxIndSensUnionFindAARaw>);
    return;
  }

  llvm_unreachable(
      "We should have handled all AnalysisType values in the switch above");
}

auto LLVMUnionFindAliasSet::getEmptyAliasSet() -> BoxedPtr<AliasSetTy> {
  static AliasSetTy EmptySet{};
  static AliasSetTy *EmptySetPtr = &EmptySet;
  return &EmptySetPtr;
}

void LLVMUnionFindAliasSet::print(llvm::raw_ostream &OS) const {
  assert(isValid());
  AARes->print(OS, Cfg);
}

void LLVMUnionFindAliasSet::printAsJson(llvm::raw_ostream &OS) const {
  // TODO
  OS << "{}\n";
}

[[nodiscard]] llvm::StringRef
psr::to_string(LLVMUnionFindAliasSet::AnalysisLocality Loc) noexcept {
  switch (Loc) {
  case LLVMUnionFindAliasSet::AnalysisLocality::Global:
    return "global";
  case LLVMUnionFindAliasSet::AnalysisLocality::FunctionLocal:
    return "local";
  }
  llvm_unreachable(
      "All analysis-localities should be handled in the switch above");
}
