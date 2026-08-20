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
#include "phasar/PhasarLLVM/Pointer/LLVMRawAAResults.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/BottomupUnionFindAA.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/IR/Instructions.h"

#include <memory>

namespace psr {
extern template class CallingContextSensUnionFindAA<LLVMPAGDomain>;
extern template class IndirectionSensUnionFindAA<LLVMPAGDomain>;
extern template class BottomupUnionFindAA<LLVMPAGDomain>;

/// For backwards-comppatibility only
template <typename AAResT>
using LLVMUnionFindAliasIterator = LLVMRawAliasIterator<AAResT>;

/// For backwards-comppatibility only
template <typename AAResT>
using LLVMLocalUnionFindAliasIterator = LLVMLocalRawAliasIterator<AAResT>;

namespace pag {
/// Utility class to make pag::PBMixin<IndirectionSensUnionFindAA,
/// LLVMCGProvider> implement PBStrategy.
class LLVMCGProvider : public LLVMPAGDomain {
public:
  constexpr LLVMCGProvider(
      NonNullPtr<const LLVMBasedCallGraph> CG PSR_LIFETIMEBOUND) noexcept
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
[[nodiscard]] inline RawAAResult auto computeUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, AnalysisT &&Ana,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
    PAGBuilderImpl Impl = LLVMPAGBuilder::withBuiltinMemSSA()) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }

  Impl.buildPAG(IRDB, *VC, &Ana);

  const auto NumVars = VC->size();
  return PSR_FWD(Ana).consumeAAResults(NumVars, [&VC](ValueId VId) {
    return llvm::any_of(VC->id2vars(VId),
                        [](PAGVariable V) { return !V.isReturnVariable(); });
  });
}

template <pag::CanOnAddEdge AnalysisT,
          std::derived_from<PAGBuilder<LLVMPAGDomain>> PAGBuilderImpl =
              LLVMPAGBuilder>
[[nodiscard]] inline RawAAResult auto computeUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, AnalysisT &&Ana, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr,
    PAGBuilderImpl Impl = LLVMPAGBuilder::withBuiltinMemSSA()) {
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
[[nodiscard]] RawAAResultIntersection<CallingContextSensUnionFindAAResult,
                                      BasicUnionFindAAResult>
computeCtxIndSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC = nullptr);
[[nodiscard]] RawAAResultIntersection<BasicUnionFindAAResult,
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
[[nodiscard]] inline IsLLVMAliasIterator auto computeUnionFindAA(
    const LLVMProjectIRDB &IRDB, AnalysisT &&Ana,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC PSR_LIFETIMEBOUND = nullptr,
    PAGBuilderImpl Impl = LLVMPAGBuilder::withBuiltinMemSSA()) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }

  auto AARes =
      computeUnionFindAARaw(IRDB, PSR_FWD(Ana), VC.get(), std::move(Impl));
  return LLVMRawAliasIterator{
      std::move(AARes),
      std::move(VC),
  };
}

template <pag::CanOnAddEdge AnalysisT,
          std::derived_from<PAGBuilder<LLVMPAGDomain>> PAGBuilderImpl =
              LLVMPAGBuilder>
[[nodiscard]] inline IsLLVMAliasIterator auto computeUnionFindAA(
    const LLVMProjectIRDB &IRDB, AnalysisT &&Ana, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC PSR_LIFETIMEBOUND = nullptr,
    PAGBuilderImpl Impl = LLVMPAGBuilder::withBuiltinMemSSA()) {
  auto Strategy = pag::PBMixin{
      PSR_FWD(Ana),
      pag::LLVMCGProvider{&CG},
  };
  return computeUnionFindAA(IRDB, Strategy, std::move(VC), std::move(Impl));
}

[[nodiscard]] LLVMRawAliasIterator<CallingContextSensUnionFindAAResult>
computeCtxSensUnionFindAA(const LLVMProjectIRDB &IRDB,
                          const LLVMBasedCallGraph &CG,
                          MaybeUniquePtr<ValueCompressor<PAGVariable>> VC
                              PSR_LIFETIMEBOUND = nullptr);
[[nodiscard]] LLVMRawAliasIterator<BasicUnionFindAAResult>
computeBotCtxSensUnionFindAA(const LLVMProjectIRDB &IRDB,
                             const LLVMBasedCallGraph &CG,
                             MaybeUniquePtr<ValueCompressor<PAGVariable>> VC
                                 PSR_LIFETIMEBOUND = nullptr);
[[nodiscard]] LLVMRawAliasIterator<BasicUnionFindAAResult>
computeIndSensUnionFindAA(const LLVMProjectIRDB &IRDB,
                          const LLVMBasedCallGraph &CG,
                          MaybeUniquePtr<ValueCompressor<PAGVariable>> VC
                              PSR_LIFETIMEBOUND = nullptr);
[[nodiscard]] LLVMRawAliasIterator<RawAAResultIntersection<
    CallingContextSensUnionFindAAResult, BasicUnionFindAAResult>>
computeCtxIndSensUnionFindAA(const LLVMProjectIRDB &IRDB,
                             const LLVMBasedCallGraph &CG,
                             MaybeUniquePtr<ValueCompressor<PAGVariable>> VC
                                 PSR_LIFETIMEBOUND = nullptr);
[[nodiscard]] LLVMRawAliasIterator<
    RawAAResultIntersection<BasicUnionFindAAResult, BasicUnionFindAAResult>>
computeBotCtxIndSensUnionFindAA(const LLVMProjectIRDB &IRDB,
                                const LLVMBasedCallGraph &CG,
                                MaybeUniquePtr<ValueCompressor<PAGVariable>> VC
                                    PSR_LIFETIMEBOUND = nullptr);

} // namespace psr
