#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/ControlFlow/CallGraphBase.h"
#include "phasar/Pointer/PointerAssignmentGraph.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/DebugOutput.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/SCCGeneric.h"
#include "phasar/Utils/SCCId.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace psr {

struct BottomupUnionFindAABase {
  static constexpr llvm::StringLiteral LogCategory = "BottomupUnionFindAA";
};

/// SCC-based, bottom-up union-find alias analysis.
///
/// Interprocedural Steensgaard-style analysis with summary propagation along
/// the SCC DAG of the call graph.
///
/// When \p SoundnessFlag is \c Soundness::Soundy (default), a top-down pass
/// propagates parameter aliasing information back to callers for soundness.
/// Set it to \c Soundness::Unsound to skip this pass and trade precision for
/// speed.
///
/// Intra-SCC Call/Return edges (mutual recursion) are handled
/// context-insensitively.
///
/// Implements \c pag::PBStrategy. After \c buildPAG() completes, call
/// \c consumeAAResults() to obtain a \c BasicUnionFindAAResult.
///
/// \tparam AnalysisDomainT The analysis domain, e.g., \c LLVMPAGDomain.
/// \tparam SoundnessFlag   Controls whether the top-down parameter-aliasing
///                         pass is performed.
template <typename AnalysisDomainT, Soundness SoundnessFlag = Soundness::Soundy>
class BottomupUnionFindAA : BottomupUnionFindAABase {
  struct ParamInfo {
    ValueId Arg;
    ValueId Param;
    pag::Call CS;
  };
  struct CallSiteInfo {
    llvm::SmallVector<ParamInfo, 2> Params{};
    llvm::SmallVector<ValueId, 1> Ret{};
    std::optional<ValueId> RetSlot{};
  };

public:
  using n_t = typename AnalysisDomainT::n_t;
  using v_t = typename AnalysisDomainT::v_t;
  using f_t = typename AnalysisDomainT::f_t;
  using db_t = typename AnalysisDomainT::db_t;

  using CGTy = CallGraph<n_t, f_t>;
  using RevCGTy = ReverseCGGraph<CGTy, db_t>;
  using FunVtxTy = typename RevCGTy::FunctionId;

  using BottomupUnionFindAABase::LogCategory;

  static constexpr auto InvalidSCC = SCCId<FunVtxTy>(UINT32_MAX);

  constexpr BottomupUnionFindAA(RevCGTy &&RevCG,
                                NonNullPtr<SCCHolder<FunVtxTy>> CGSCCs)
      : RevCG(std::move(RevCG)), CGSCCs(CGSCCs.get()) {
    const auto NumSCCs = CGSCCs->size();
    Calls.resize(NumSCCs);
    IdsForSum.resize(NumSCCs);
  }

  constexpr BottomupUnionFindAA(RevCGTy &&RevCG) : RevCG(std::move(RevCG)) {
    auto SCCs = computeSCCs(this->RevCG);
    this->CGSCCs = std::make_unique<SCCHolder<FunVtxTy>>(std::move(SCCs));
    const auto NumSCCs = CGSCCs->size();
    Calls.resize(NumSCCs);
    IdsForSum.resize(NumSCCs);
  }

  void withCalleesOfCallAt(ByConstRef<n_t> CS,
                           std::invocable<f_t> auto WithCallee) const {
    for (const auto &Callee : RevCG.CGView->getCalleesOfCallAt(CS)) {
      std::invoke(WithCallee, Callee);
    }
  }

  void onAddEdge(ValueId From, ValueId To, pag::Edge E,
                 Nullable<n_t> CallSite) {
    if (E.isa<pag::Call>()) {
      auto FromSCC = SCCOfVal[From];
      auto ToSCC = SCCOfVal[To];
      if (FromSCC != ToSCC && FromSCC != InvalidSCC && ToSCC != InvalidSCC) {
        Calls[FromSCC][CallSite].Params.emplace_back(From, To,
                                                     E.cast<pag::Call>());
        IdsForSum[ToSCC].insert(To);
        return;
      }
      // fallthrough -- intra-SCC edges are context-insensitive
    } else if (E.isa<pag::Return>()) {
      auto FromSCC = SCCOfVal[From];
      auto ToSCC = SCCOfVal[To];
      if (FromSCC != ToSCC && FromSCC != InvalidSCC && ToSCC != InvalidSCC) {
        auto &RetInfo = Calls[ToSCC][CallSite];
        assert(RetInfo.RetSlot == std::nullopt || RetInfo.RetSlot == To);
        RetInfo.RetSlot = To;
        RetInfo.Ret.push_back(From);

        IdsForSum[FromSCC].insert(From);
        return;
      }
      // fallthrough -- intra-SCC edges are context-insensitive
    }

    Base.onAddEdge(From, To, E, CallSite);
  }

  void onAddValue(ByConstRef<v_t> Var, ValueId VId) {
    auto &SccPlace = SCCOfVal.emplace_back(InvalidSCC);
    if (auto &&Fun = getFunction(Var)) {
      if (auto FunVtx = RevCG.FC.getOrNull(Fun)) {
        SccPlace = CGSCCs->SCCOfNode[*FunVtx];
      }
    }

    Base.onAddValue(Var, VId);
  }

  [[nodiscard]] BasicUnionFindAAResult consumeAAResults(size_t NumVars) && {

    bottomupPropagation(NumVars);

    if constexpr (SoundnessFlag != Soundness::Unsound) {
      // For soundness, we need to handle parameter aliasing
      topdownPropagation();
    }

    return std::move(Base).consumeAAResults(NumVars);
  }

private:
  void bottomupPropagation(size_t NumVars) {
    TypedVector<ValueId, RawAliasSet<ValueId>> IntermediateBackView(NumVars);

    for (SCCId<FunVtxTy> CurrSCC :
         llvm::reverse(iota<SCCId<FunVtxTy>>(CGSCCs->size()))) {

      for (const auto &[CS, CSInfo] : Calls[CurrSCC]) {
        PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "At CS: " << NToString(CS));
        llvm::SmallDenseMap<ValueId, llvm::SmallVector<ValueId, 2>> BackMap;

        for (const auto &[Arg, Param, E] : CSInfo.Params) {
          auto ParamRep = Base.AliasSets.find(Param);
          BackMap[ParamRep].push_back(Arg);
        }
        if (CSInfo.RetSlot) {
          for (auto Ret : CSInfo.Ret) {
            auto RetRep = Base.AliasSets.find(Ret);
            BackMap[RetRep].push_back(*CSInfo.RetSlot);
          }
        }

        // --- Apply summaries:
        for (const auto &[Param, Args] : BackMap) {
          PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                               "  Param: " << psr::to_underlying(Param)
                                           << "; Args: "
                                           << PrettyPrinter{Args});

          const auto &ToAliases = IntermediateBackView[Param];
          ToAliases.foreach ([&](ValueId Alias) {
            auto AliasRep = Base.AliasSets.find(Alias);
            if (const auto *AliasFroms = getOrNull(BackMap, AliasRep)) {
              for (auto Arg : Args) {
                PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                                     "  JOIN " << psr::to_underlying(Arg)
                                               << " WITH "
                                               << PrettyPrinter{*AliasFroms});

                for (auto Alias : *AliasFroms) {
                  Base.AliasSets.join(Arg, Alias);
                }
              }
            }
          });
        }
      }
      // --- Compute new summaries:

      IdsForSum[CurrSCC].foreach ([&](ValueId SumId) {
        auto SumIdRep = Base.AliasSets.find(SumId);
        IntermediateBackView[SumIdRep].insert(SumId);
      });
    }
  }

  void topdownPropagation() {
    for (SCCId<FunVtxTy> CurrSCC : iota<SCCId<FunVtxTy>>(CGSCCs->size())) {
      for (const auto &[CS, CSInfo] : Calls[CurrSCC]) {
        const size_t Sz = CSInfo.Params.size();
        for (size_t I : psr::iota(Sz)) {
          const ParamInfo FirstInfo = CSInfo.Params[I];
          const auto FirstArgRep = Base.AliasSets.find(FirstInfo.Arg);
          for (size_t J : psr::iota(I + 1, Sz)) {
            const ParamInfo &SecondInfo = CSInfo.Params[J];
            const auto SecondArgRep = Base.AliasSets.find(SecondInfo.Arg);

            if (FirstArgRep == SecondArgRep) {
              // Found parameter-aliasing!
              Base.AliasSets.join(FirstInfo.Param, SecondInfo.Param);
            }
          }
        }
      }
    }
  }

  [[nodiscard]] Nullable<f_t> getFunction(ByConstRef<v_t> Var) {
    if constexpr (requires() { getPointerFrom(Var)->getFunction(); }) {
      return getPointerFrom(Var)->getFunction();
    } else {
      return RevCG.IRDB->getFunctionOf(Var);
    }
  }

  RevCGTy RevCG;
  MaybeUniquePtr<SCCHolder<FunVtxTy>> CGSCCs;

  TypedVector<ValueId, SCCId<FunVtxTy>> SCCOfVal;
  TypedVector<SCCId<FunVtxTy>, std::unordered_map<n_t, CallSiteInfo>> Calls;
  TypedVector<SCCId<FunVtxTy>, RawAliasSet<ValueId>> IdsForSum;
  BasicUnionFindAA<AnalysisDomainT> Base{};
};
} // namespace psr
