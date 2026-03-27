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
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/BottomupUnionFindAA.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include <memory>
#include <type_traits>

namespace psr {
extern template class CallingContextSensUnionFindAA<LLVMPAGDomain>;
extern template class IndirectionSensUnionFindAA<LLVMPAGDomain>;
extern template class BottomupUnionFindAA<LLVMPAGDomain>;

/// Returns a \c ValueId handler suitable for \c RawAliasSet::foreach() that
/// maps each alias \c ValueId back to all of its underlying \c llvm::Value*
/// (via \p VC), forwarding non-null values to \p Callback.
constexpr std::invocable<ValueId> auto
llvmUnionFindAliasHandler(const ValueCompressor<PAGVariable> &VC,
                          std::invocable<const llvm::Value *> auto Callback) {
  return [&VC, Callback{copyOrRef(Callback)}](ValueId Alias) {
    for (auto V : VC.id2vars(Alias)) {
      if (const auto *LLVMVar = V.valueOrNull()) [[likely]] {
        std::invoke(Callback, LLVMVar);
      }
    }
  };
}

namespace pag {
/// Utility class to make pag::PBMixin<IndirectionSensUnionFindAA,
/// LLVMCGProvider> implement PBStrategy.
class LLVMCGProvider : public LLVMPAGDomain {
public:
  constexpr LLVMCGProvider(NonNullPtr<const LLVMBasedCallGraph> CG) noexcept
      : CG(CG) {}

  void withCalleesOfCallAt(n_t Inst,
                           std::invocable<f_t> auto WithCallee) const {
    for (const auto *Callee : CG->getCalleesOfCallAt(Inst)) {
      std::invoke(WithCallee, Callee);
    }
  }

private:
  NonNullPtr<const LLVMBasedCallGraph> CG;
};
} // namespace pag

/// CRTP mixin that adds the LLVM alias-iterator interface to a class that
/// holds a \c UnionFindAAResult and a \c ValueCompressor<PAGVariable>.
///
/// Provides \c forallAliasesOf(), \c mayAlias(), and \c alias() overloads
/// accepting both \c llvm::Value* and \c ValueId arguments.  Results are
/// reported as \c llvm::Value* via the stored \c ValueCompressor.
///
/// The derived class must expose a \c VC member (pointer to a
/// \c ValueCompressor<PAGVariable>).
///
/// \tparam Derived The CRTP derived class.
/// \tparam AAResT  A type satisfying \c UnionFindAAResult.
template <typename Derived, typename AAResT>
  requires UnionFindAAResult<std::remove_cvref_t<AAResT>>
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct LLVMUnionFindAliasIteratorMixin {
  [[no_unique_address]] AAResT AARes;

  using v_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;

  [[nodiscard]] decltype(auto) getRawAliasSet(ValueId ValId) const {
    return AARes.getRawAliasSet(ValId);
  }

  [[nodiscard]] const auto &base() const noexcept { return AARes; }

  void
  forallAliasesOf(ValueId VId, const auto & /*Inst*/,
                  std::invocable<const llvm::Value *> auto Callback) const {
    const auto &RawAliases = AARes.getRawAliasSet(VId);
    RawAliases.foreach (
        llvmUnionFindAliasHandler(*self().VC, copyOrRef(Callback)));
  }

  void
  forallAliasesOf(const llvm::Value *Ptr, const auto &Inst,
                  std::invocable<const llvm::Value *> auto Callback) const {
    if (auto ValId = self().VC->getOrNull(Ptr)) {
      forallAliasesOf(*ValId, Inst, copyOrRef(Callback));
    }
  }

  [[nodiscard]] bool mayAlias(ValueId Ptr1, ValueId Ptr2) const {
    return AARes.mayAlias(Ptr1, Ptr2);
  }

  [[nodiscard]] bool mayAlias(ValueId Ptr1, ValueId Ptr2,
                              const auto & /*AtInstruction*/) const {
    return AARes.mayAlias(Ptr1, Ptr2);
  }

  [[nodiscard]] bool mayAlias(const llvm::Value *Ptr1,
                              const llvm::Value *Ptr2) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);

    return ValId1 && ValId2 && mayAlias(*ValId1, *ValId2);
  }

  [[nodiscard]] bool mayAlias(const llvm::Value *Ptr1, const llvm::Value *Ptr2,
                              const auto & /*AtInstruction*/) const {
    return mayAlias(Ptr1, Ptr2);
  }

  [[nodiscard]] AliasResult alias(const llvm::Value *Ptr1,
                                  const llvm::Value *Ptr2,
                                  const auto &AtInstruction) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);
    if (!ValId1 || !ValId2) {
      return AliasResult::NoAlias;
    }
    if (*ValId1 == *ValId2) {
      // XXX: Check exactly the conditions for must alias!
      return !llvm::isa<llvm::LoadInst>(Ptr1) &&
                     !llvm::isa<llvm::LoadInst>(Ptr2)
                 ? AliasResult::MustAlias
                 : AliasResult::MayAlias;
    }
    return mayAlias(*ValId1, *ValId2, AtInstruction) ? AliasResult::MayAlias
                                                     : AliasResult::NoAlias;
  }

  [[nodiscard]] constexpr const Derived &self() const noexcept {
    return *static_cast<const Derived *>(this);
  }
};

template <typename AAResT>
  requires UnionFindAAResult<std::remove_cvref_t<AAResT>>
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct LLVMUnionFindAliasIterator
    : public LLVMUnionFindAliasIteratorMixin<LLVMUnionFindAliasIterator<AAResT>,
                                             AAResT> {
  MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC;

  constexpr LLVMUnionFindAliasIterator(
      AAResT &&AARes, MaybeUniquePtr<const ValueCompressor<PAGVariable>> VC)
      : psr::LLVMUnionFindAliasIteratorMixin<LLVMUnionFindAliasIterator<AAResT>,
                                             AAResT>{PSR_FWD(AARes)},
        VC(std::move(VC)) {}
};

template <typename AAResT>
LLVMUnionFindAliasIterator(AAResT,
                           MaybeUniquePtr<const ValueCompressor<PAGVariable>>)
    -> LLVMUnionFindAliasIterator<AAResT>;
template <typename AAResT>
LLVMUnionFindAliasIterator(AAResT, const ValueCompressor<PAGVariable> *)
    -> LLVMUnionFindAliasIterator<AAResT>;

namespace detail {
class LLVMLocalUnionFindAliasIteratorBase {
public:
  explicit LLVMLocalUnionFindAliasIteratorBase(
      const ValueCompressor<PAGVariable> &VC);

protected:
  llvm::DenseMap<const llvm::Function *, RawAliasSet<ValueId>> GlobalsOrInFun;
};
} // namespace detail

/// CRTP mixin adding a function-local view on top of a global
/// \c UnionFindAAResult.
///
/// Extends \c LLVMUnionFindAliasIteratorMixin with \c forallAliasesOf()
/// overloads that accept an \c llvm::Function* or \c llvm::Instruction*
/// context.  When a non-null context is provided, alias sets are intersected
/// with the set of variables that are visible in that function (globals plus
/// locals defined in that function), giving a function-local result even
/// though the underlying analysis is interprocedural.
template <typename Derived, typename AAResT>
class LLVMLocalUnionFindAliasIteratorMixin
    : public detail::LLVMLocalUnionFindAliasIteratorBase {
public:
  LLVMLocalUnionFindAliasIteratorMixin(AAResT &&AARes,
                                       const ValueCompressor<PAGVariable> &VC)
      : detail::LLVMLocalUnionFindAliasIteratorBase(VC), AARes(PSR_FWD(AARes)) {
  }

  [[nodiscard]] decltype(auto) getRawAliasSet(ValueId ValId) const {
    return AARes.getRawAliasSet(ValId);
  }

  [[nodiscard]] auto getRawAliasSet(ValueId ValId,
                                    const llvm::Function *Context) const {
    auto Vars = AARes.getRawAliasSet(ValId);
    if (Context) {
      Vars &= getOrDefault(GlobalsOrInFun, Context);
    }
    return Vars;
  }

  [[nodiscard]] const auto &base() const noexcept { return AARes; }

  [[nodiscard]] auto getRawAliasSet(ValueId ValId,
                                    const llvm::Instruction *Context) const {
    return getRawAliasSet(ValId, getFunction(Context));
  }

  void forallAliasesOf(ValueId VId, const llvm::Function *Context,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    const auto AliasHandler =
        llvmUnionFindAliasHandler(*self().VC, copyOrRef(WithAlias));

    auto &&RawVars = AARes.getRawAliasSet(VId);
    if (Context) {
      auto Vars = PSR_FWD(RawVars);
      Vars &= getOrDefault(GlobalsOrInFun, Context);
      Vars.foreach (AliasHandler);
    } else {
      RawVars.foreach (AliasHandler);
    }
  }

  void forallAliasesOf(const llvm::Value *Val, const llvm::Function *Context,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    if (auto ValId = self().VC->getOrNull(Val)) {
      forallAliasesOf(*ValId, Context, copyOrRef(WithAlias));
    }
  }

  void forallAliasesOf(ValueId ValId, const llvm::Instruction *AtInstruction,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    forallAliasesOf(ValId, psr::getFunction(AtInstruction),
                    copyOrRef(WithAlias));
  }

  void forallAliasesOf(const llvm::Value *Val,
                       const llvm::Instruction *AtInstruction,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    forallAliasesOf(Val, psr::getFunction(AtInstruction), copyOrRef(WithAlias));
  }

  void forallAliasesOf(const llvm::Value *Val,
                       std::invocable<const llvm::Value *> auto WithAlias) {
    forallAliasesOf(Val, psr::getFunction(Val), copyOrRef(WithAlias));
  }

  [[nodiscard]] bool
  mayAlias(ValueId ValId1, ValueId ValId2,
           const llvm::Instruction * /*AtInstruction*/ = nullptr) const {
    // XXX: Should we filter by AtInstruction-context here as well?
    return AARes.mayAlias(ValId1, ValId2);
  }

  [[nodiscard]] bool
  mayAlias(const llvm::Value *Ptr1, const llvm::Value *Ptr2,
           const llvm::Instruction * /*AtInstruction*/ = nullptr) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);

    // XXX: Should we filter by AtInstruction-context here as well?
    return ValId1 && ValId2 && AARes.mayAlias(*ValId1, *ValId2);
  }

  [[nodiscard]] AliasResult alias(const llvm::Value *Ptr1,
                                  const llvm::Value *Ptr2,
                                  const auto &AtInstruction) const {
    auto ValId1 = self().VC->getOrNull(Ptr1);
    auto ValId2 = self().VC->getOrNull(Ptr2);
    if (!ValId1 || !ValId2) {
      return AliasResult::NoAlias;
    }
    if (*ValId1 == *ValId2) {
      // XXX: Check exactly the conditions for must alias!
      return !llvm::isa<llvm::LoadInst>(Ptr1) &&
                     !llvm::isa<llvm::LoadInst>(Ptr2)
                 ? AliasResult::MustAlias
                 : AliasResult::MayAlias;
    }
    return mayAlias(*ValId1, *ValId2, AtInstruction) ? AliasResult::MayAlias
                                                     : AliasResult::NoAlias;
  }

  [[nodiscard]] constexpr const Derived &self() const noexcept {
    return *static_cast<const Derived *>(this);
  }

private:
  AAResT AARes;
};

template <typename AAResT>
class LLVMLocalUnionFindAliasIterator
    : public LLVMLocalUnionFindAliasIteratorMixin<
          LLVMLocalUnionFindAliasIterator<AAResT>, AAResT> {
  friend LLVMLocalUnionFindAliasIteratorMixin<
      LLVMLocalUnionFindAliasIterator<AAResT>, AAResT>;

public:
  LLVMLocalUnionFindAliasIterator(
      AAResT &&AARes, NonNullPtr<const ValueCompressor<PAGVariable>> VC)
      : LLVMLocalUnionFindAliasIteratorMixin<
            LLVMLocalUnionFindAliasIterator<AAResT>, AAResT>(PSR_FWD(AARes),
                                                             *VC),
        VC(VC) {}

private:
  NonNullPtr<const ValueCompressor<PAGVariable>> VC;
};

/// Low-level entry point: builds the PAG for \p IRDB using the given
/// \p AnalysisT strategy, and returns the raw alias-analysis result (not
/// wrapped in an \c LLVMUnionFindAliasIterator).
///
/// \param IRDB   The project to analyze.
/// \param Ana    A \c PBStrategy instance (e.g., \c BasicUnionFindAA,
///               \c BottomupUnionFindAA).  Taken by forwarding reference and
///               consumed when results are extracted.
/// \param VC     Optional pre-populated \c ValueCompressor.  A new one is
///               allocated if null.
/// \param Impl   PAG builder implementation (default: \c LLVMPAGBuilder).
template <pag::PBStrategy AnalysisT,
          std::derived_from<PAGBuilder<LLVMPAGDomain>> PAGBuilderImpl =
              LLVMPAGBuilder>
[[nodiscard]] inline UnionFindAAResult auto
computeUnionFindAARaw(const LLVMProjectIRDB &IRDB, AnalysisT &&Ana,
                      MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                      PAGBuilderImpl Impl = {}) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }

  Impl.buildPAG(IRDB, *VC, &Ana);

  const auto NumVars = VC->size();
  return PSR_FWD(Ana).consumeAAResults(NumVars);
}

template <pag::CanOnAddEdge AnalysisT,
          std::derived_from<PAGBuilder<LLVMPAGDomain>> PAGBuilderImpl =
              LLVMPAGBuilder>
[[nodiscard]] inline UnionFindAAResult auto
computeUnionFindAARaw(const LLVMProjectIRDB &IRDB, AnalysisT &&Ana,
                      const LLVMBasedCallGraph &CG,
                      MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                      PAGBuilderImpl Impl = {}) {
  auto Strategy = pag::PBMixin{
      PSR_FWD(Ana),
      pag::LLVMCGProvider{&CG},
  };
  return computeUnionFindAARaw(IRDB, Strategy, std::move(VC), std::move(Impl));
}

[[nodiscard]] CallingContextSensUnionFindAAResult computeCtxSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] BasicUnionFindAAResult computeBotCtxSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] BasicUnionFindAAResult computeIndSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] UnionFindAAResultIntersection<CallingContextSensUnionFindAAResult,
                                            BasicUnionFindAAResult>
computeCtxIndSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] UnionFindAAResultIntersection<BasicUnionFindAAResult,
                                            BasicUnionFindAAResult>
computeBotCtxIndSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);

/// Builds the PAG and returns a \c LLVMUnionFindAliasIterator (owns the \c
/// ValueCompressor and the raw result) that implements the \c
/// IsLLVMAliasIterator concept.
///
/// \param IRDB   The project to analyze.
/// \param Ana    A \c PBStrategy instance. Consumed when results are extracted.
/// \param VC     Optional pre-populated \c ValueCompressor. A new one is
///               allocated if null; the iterator takes ownership.
/// \param Impl   PAG builder implementation (default: \c LLVMPAGBuilder).
template <pag::PBStrategy AnalysisT,
          std::derived_from<PAGBuilder<LLVMPAGDomain>> PAGBuilderImpl =
              LLVMPAGBuilder>
[[nodiscard]] inline IsLLVMAliasIterator auto
computeUnionFindAA(const LLVMProjectIRDB &IRDB, AnalysisT &&Ana,
                   MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                   PAGBuilderImpl Impl = {}) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }

  auto AARes =
      computeUnionFindAARaw(IRDB, PSR_FWD(Ana), VC.get(), std::move(Impl));
  return LLVMUnionFindAliasIterator{
      std::move(AARes),
      std::move(VC),
  };
}

template <pag::CanOnAddEdge AnalysisT,
          std::derived_from<PAGBuilder<LLVMPAGDomain>> PAGBuilderImpl =
              LLVMPAGBuilder>
[[nodiscard]] inline IsLLVMAliasIterator auto
computeUnionFindAA(const LLVMProjectIRDB &IRDB, AnalysisT &&Ana,
                   const LLVMBasedCallGraph &CG,
                   MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
                   PAGBuilderImpl Impl = {}) {
  auto Strategy = pag::PBMixin{
      PSR_FWD(Ana),
      pag::LLVMCGProvider{&CG},
  };
  return computeUnionFindAA(IRDB, Strategy, std::move(VC), std::move(Impl));
}

[[nodiscard]] LLVMUnionFindAliasIterator<CallingContextSensUnionFindAAResult>
computeCtxSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] LLVMUnionFindAliasIterator<BasicUnionFindAAResult>
computeBotCtxSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] LLVMUnionFindAliasIterator<BasicUnionFindAAResult>
computeIndSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] LLVMUnionFindAliasIterator<UnionFindAAResultIntersection<
    CallingContextSensUnionFindAAResult, BasicUnionFindAAResult>>
computeCtxIndSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] LLVMUnionFindAliasIterator<UnionFindAAResultIntersection<
    BasicUnionFindAAResult, BasicUnionFindAAResult>>
computeBotCtxIndSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);

} // namespace psr
