#include "phasar/PhasarLLVM/Pointer/LLVMRawAliasSet.h"

#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraph.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasInfoBase.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/EnumFlags.h"
#include "phasar/Utils/Fn.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

#include <memory>

using namespace psr;

static_assert(IsAliasInfo<LLVMRawAliasSet>);

LLVMRawAliasSet::LLVMRawAliasSet(const LLVMProjectIRDB *IRDB,
                                 const LLVMBasedCallGraph &BaseCG, Config Cfg,
                                 ValueCompressor<PAGVariable> *VC)
    : Cfg(Cfg) {
  MaybeUniquePtr<ValueCompressor<PAGVariable>> VCOwn = VC;
  if (!VC) {
    VCOwn = std::make_unique<ValueCompressor<PAGVariable>>();
  }

  auto MakeAAResModel =
      [&, IRDB, Cfg, VCOwn = std::move(VCOwn)](
          auto AAResCtor) mutable -> std::unique_ptr<AAResultConcept> {
    auto AARes = AAResCtor(*IRDB, BaseCG, VCOwn.get());
    using AAResT = decltype(AARes);

    if (Cfg.ALocality == AnalysisLocality::FunctionLocal) {
      const auto &VCRef = *VCOwn;
      return std::make_unique<
          AAResultModel<LLVMLocalRawAliasIteratorMixin, AAResT>>(
          std::move(VCOwn), PSR_FWD(AARes), VCRef);
    }

    return std::make_unique<AAResultModel<LLVMRawAliasIteratorMixin, AAResT>>(
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

auto LLVMRawAliasSet::getEmptyAliasSet() -> BoxedPtr<AliasSetTy> {
  static AliasSetTy EmptySet{};
  static AliasSetTy *EmptySetPtr = &EmptySet;
  return &EmptySetPtr;
}

void LLVMRawAliasSet::print(llvm::raw_ostream &OS) const {
  assert(isValid());
  // OS << "ValueCompressor: {\n";
  // for (const auto &[VId, Values] : AARes->VC->id2vars().enumerate()) {
  //   OS << "  #" << uint32_t(VId) << ":\n";
  //   for (const auto Val : Values) {
  //     OS << "    " << to_string(Val) << '\n';
  //   }
  // }
  AARes->print(OS, Cfg);
}

void LLVMRawAliasSet::printAsJson(llvm::raw_ostream &OS) const {
  // TODO
  OS << "{}\n";
}

[[nodiscard]] llvm::StringRef
psr::to_string(LLVMRawAliasSet::AnalysisLocality Loc) noexcept {
  switch (Loc) {
  case LLVMRawAliasSet::AnalysisLocality::Global:
    return "global";
  case LLVMRawAliasSet::AnalysisLocality::FunctionLocal:
    return "local";
  }
  llvm_unreachable(
      "All analysis-localities should be handled in the switch above");
}
