#pragma once

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
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace psr {
template <typename AnalysisDomainT> class BottomupUnionFindAA {
  struct CallSiteInfo {
    llvm::SmallVector<std::tuple<ValueId, ValueId, pag::Call>, 2> Params{};
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
      return;
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
    TypedVector<ValueId, RawAliasSet<ValueId>> IntermediateBackView(NumVars);

    for (SCCId<FunVtxTy> CurrSCC :
         llvm::reverse(iota<SCCId<FunVtxTy>>(CGSCCs->size()))) {

      for (const auto &[CS, CSInfo] : Calls[CurrSCC]) {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "BottomupUnionFindAA",
                             "At CS: " << NToString(CS));
        llvm::SmallDenseMap<ValueId, llvm::SmallVector<ValueId, 2>> BackMap;

        for (const auto &[Arg, Param, E] : CSInfo.Params) {
          BackMap[Param].push_back(Arg);
        }
        if (CSInfo.RetSlot) {
          for (auto Ret : CSInfo.Ret) {
            BackMap[Ret].push_back(*CSInfo.RetSlot);
          }
        }

        // --- Apply summaries:
        for (const auto &[Param, Args] : BackMap) {
          PHASAR_LOG_LEVEL_CAT(DEBUG, "BottomupUnionFindAA",
                               "  Param: " << psr::to_underlying(Param)
                                           << "; Args: "
                                           << PrettyPrinter{Args});

          const auto &ToAliases = IntermediateBackView[Param];
          ToAliases.foreach ([&](ValueId Alias) {
            if (const auto *AliasFroms = getOrNull(BackMap, Alias)) {
              for (auto Arg : Args) {
                PHASAR_LOG_LEVEL_CAT(DEBUG, "BottomupUnionFindAA",
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

    return std::move(Base).consumeAAResults(NumVars);
  }

private:
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
