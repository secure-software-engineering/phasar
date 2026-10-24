#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/WPDS/RuleProvider.h"
#include "phasar/DataFlow/WPDS/Solver/WPDSSolverResults.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/SemiRing.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

#include <tuple>
#include <utility>

namespace psr {

template <wpds::RuleProvider RuleProviderT, IsSemiRing SemiRingT>
  requires std::same_as<typename RuleProviderT::weight_type,
                        typename SemiRingT::EdgeFunctionType>
class WPDSSolver : private wpds::detail::SolverResultsData<
                       typename RuleProviderT::control_location_type,
                       typename RuleProviderT::stack_element_type,
                       typename RuleProviderT::weight_type> {
  using base_t = wpds::detail::SolverResultsData<
      typename RuleProviderT::control_location_type,
      typename RuleProviderT::stack_element_type,
      typename RuleProviderT::weight_type>;

  using base_t::ESGNodeCompressor;
  using base_t::Incoming;
  using base_t::PathEdges;
  using base_t::Seeds;

public:
  using cl_t = typename RuleProviderT::control_location_type;
  using se_t = typename RuleProviderT::stack_element_type;
  using weight_t = typename RuleProviderT::weight_type;
  using path_edge_t = std::tuple<wpds::ESGNodeId, se_t, cl_t>;

  static constexpr llvm::StringLiteral LogCategory = "RuleBasedSolver";

  explicit WPDSSolver(RuleProviderT *RP, SemiRingT *SR) noexcept
      : RP(RP), SR(SR) {}
  explicit WPDSSolver(NonNullPtr<RuleProviderT> RP,
                      NonNullPtr<SemiRingT> SR) noexcept
      : RP(RP), SR(SR) {}
  WPDSSolver(RuleProviderT *RP, std::nullptr_t SR) = delete;
  WPDSSolver(std::nullptr_t RP, SemiRingT *SR) = delete;
  WPDSSolver(std::nullptr_t RP, std::nullptr_t SR) = delete;

  wpds::SolverResults<cl_t, se_t, weight_t> solve() & {
    solveImpl();
    return getSolverResults();
  }
  wpds::OwningSolverResults<cl_t, se_t, weight_t> solve() && {
    solveImpl();
    return consumeSolverResults();
  }

  [[nodiscard]] wpds::SolverResults<cl_t, se_t, weight_t>
  getSolverResults() const noexcept PSR_LIFETIMEBOUND {
    return {this};
  }

  [[nodiscard]] wpds::OwningSolverResults<cl_t, se_t, weight_t>
  consumeSolverResults() noexcept {
    return {std::move(static_cast<base_t &>(*this))};
  }

private:
  void solveImpl() {
    submitInitialSeeds();

    while (!WL.empty()) {
      auto [CurrPE, CurrWeight] = WL.pop_back_val();
      auto &&[CurrSrc, CurrTarget, CurrTgtFact] = CurrPE;

      for (const auto &[SuccCL, SuccSE, SuccWeight] :
           RP->getNormalRules(CurrTgtFact, CurrTarget)) {
        propagate({CurrSrc, SuccSE, SuccCL},
                  SR->extend(CurrWeight, SuccWeight));
      }

      for (const auto &[SuccFact, RetSite, EntrySE, PushWeight] :
           RP->getPushRules(CurrTgtFact, CurrTarget)) {

        if constexpr (requires() { RP->makeSrcFacts(SuccFact); }) {
          for (const auto &RealSuccFact : RP->makeSrcFacts(SuccFact)) {
            auto Entry = compressNode(RealSuccFact, EntrySE);

            propagate({Entry, EntrySE, SuccFact}, SR->identity());
            auto IncWeight = SR->extend(CurrWeight, PushWeight);
            addIncoming(Entry, CurrSrc, RetSite, IncWeight);
            applyEndSummary(Entry, CurrSrc, RetSite, IncWeight);
          }
        } else {

          auto Entry = compressNode(SuccFact, EntrySE);

          propagate({Entry, EntrySE, SuccFact}, SR->identity());
          auto IncWeight = SR->extend(CurrWeight, PushWeight);
          addIncoming(Entry, CurrSrc, RetSite, IncWeight);
          applyEndSummary(Entry, CurrSrc, RetSite, IncWeight);
        }
      }

      if constexpr (wpds::CanInjectAdditionalPushEdges<RuleProviderT>) {
        RP->injectAdditionalPushEdges(
            CurrTgtFact, CurrTarget,
            [&](llvm::ArrayRef<std::tuple<cl_t, se_t, se_t>> Edges,
                weight_t W) {
              if (!Edges.empty()) {
                PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                                     "injectAdditionalPushEdges:");
                PHASAR_LOG_LEVEL_CAT(
                    DEBUG, LogCategory,
                    "injectAdditionalPushEdges:   > For CurrTgtFact: "
                        << DToString(CurrTgtFact) << " AT "
                        << NToString(CurrTarget));
              }

              if constexpr (requires() {
                              RP->makeSrcFacts(std::get<0>(Edges.front()));
                            }) {
                // TODO: This is extremely ugly, but it works...
                const auto &[FirstCL, FirstEntrySE, FirstRetSite] =
                    Edges.front();
                for (const auto &RealCL : RP->makeSrcFacts(FirstCL)) {
                  auto TopmostSrc = CurrSrc;

                  PHASAR_LOG_LEVEL_CAT(
                      DEBUG, LogCategory,
                      "  > CL" << DToString(RealCL)
                               << "; EntrySE: " << NToString(FirstEntrySE)
                               << "; RetSite: " << NToString(FirstRetSite));

                  auto Entry = compressNode(RealCL, FirstEntrySE);

                  propagate({Entry, FirstEntrySE, FirstCL}, SR->identity());
                  auto IncWeight =
                      SR->extend(CurrWeight, std::exchange(W, SR->identity()));
                  addIncoming(Entry, TopmostSrc, FirstRetSite, IncWeight);
                  applyEndSummary(Entry, TopmostSrc, FirstRetSite, IncWeight);

                  TopmostSrc = Entry;

                  for (auto &&[CL, EntrySE, RetSite] : Edges.drop_front()) {

                    PHASAR_LOG_LEVEL_CAT(
                        DEBUG, LogCategory,
                        "  > CL" << DToString(CL)
                                 << "; EntrySE: " << NToString(EntrySE)
                                 << "; RetSite: " << NToString(RetSite));

                    auto Entry = compressNode(CL, EntrySE);

                    propagate({Entry, EntrySE, CL}, SR->identity());
                    auto IncWeight = SR->extend(
                        CurrWeight, std::exchange(W, SR->identity()));
                    addIncoming(Entry, TopmostSrc, RetSite, IncWeight);
                    applyEndSummary(Entry, TopmostSrc, RetSite, IncWeight);

                    TopmostSrc = Entry;
                  }
                }
              } else {
                auto TopmostSrc = CurrSrc;
                for (auto &&[CL, EntrySE, RetSite] : Edges) {

                  PHASAR_LOG_LEVEL_CAT(
                      DEBUG, LogCategory,
                      "  > CL" << DToString(CL)
                               << "; EntrySE: " << NToString(EntrySE)
                               << "; RetSite: " << NToString(RetSite));

                  auto Entry = compressNode(CL, EntrySE);

                  propagate({Entry, EntrySE, CL}, SR->identity());
                  auto IncWeight =
                      SR->extend(CurrWeight, std::exchange(W, SR->identity()));
                  addIncoming(Entry, TopmostSrc, RetSite, IncWeight);
                  applyEndSummary(Entry, TopmostSrc, RetSite, IncWeight);

                  TopmostSrc = Entry;
                }
              }
            });
      }

      if (RP->hasPopRules(CurrTgtFact, CurrTarget)) {

        if (!addEndSummary(CurrSrc, CurrTgtFact, CurrTarget, CurrWeight)) {
          continue;
        }

        const auto &[_, CurrSource] = ESGNodeCompressor[CurrSrc];
        for (const auto &[CSNode, IncWeight] : Incoming[CurrSrc]) {
          auto &&[CSSrc, RetSite] = CSNode;
          auto SumExtWeight = SR->extend(IncWeight, CurrWeight);
          for (const auto &[PopCL, PopWeight] :
               RP->getPopRules(CurrTgtFact, CurrTarget, RetSite, CurrSource)) {
            PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "Summary Edge:");
            propagate({CSSrc, RetSite, PopCL},
                      SR->extend(SumExtWeight, PopWeight));
          }
        }
      }
    }
  }

  [[nodiscard]] wpds::ESGNodeId compressNode(cl_t CL, se_t SE) {
    auto [Id, Inserted] =
        ESGNodeCompressor.insert({std::move(CL), std::move(SE)});
    if (Inserted) {
      Incoming.emplace_back();
      EndSummary.emplace_back();
    }
    return Id;
  }

  void submitInitialSeeds() {
    for (const auto &[InitCL, InitSE, InitWeight] : RP->initialSeeds()) {
      auto Init = compressNode(InitCL, InitSE);
      propagate({Init, InitSE, InitCL}, SR->identity());

      auto [It, Inserted] = Seeds.try_emplace(Init, InitWeight);
      if (!Inserted) {
        It->second = SR->combine(It->second, InitWeight);
      }
    }
  }

  void propagate(path_edge_t PE, weight_t Weight) {
    auto &&[PESrc, SE, CL] = PE;

    auto [It, Inserted] =
        PathEdges[std::pair{CL, SE}].try_emplace(PESrc, Weight);
    if (!Inserted) {
      Weight = SR->combine(It->second, Weight);
      if (It->second != Weight) {
        It->second = Weight;
        Inserted = true;

        PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                             "[propagate]: UPDATED ("
                                 << NToString(ESGNodeCompressor[PESrc].second)
                                 << ", "
                                 << DToString(ESGNodeCompressor[PESrc].first)
                                 << ")\t-->\t(" << NToString(SE) << ", "
                                 << DToString(CL) << ") w/ " << It->second);
      }
    } else {
      PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                           "[propagate]: NEW ("
                               << NToString(ESGNodeCompressor[PESrc].second)
                               << ", "
                               << DToString(ESGNodeCompressor[PESrc].first)
                               << ")\t-->\t(" << NToString(SE) << ", "
                               << DToString(CL) << ") w/ " << It->second);
    }

    if (Inserted) {
      WL.emplace_back(std::move(PE), std::move(Weight));
    }
  }

  void addIncoming(wpds::ESGNodeId PushNod, wpds::ESGNodeId CSSrc, se_t RetSite,
                   weight_t PushWeight) {

    PHASAR_LOG_LEVEL_CAT(
        DEBUG, LogCategory,
        "[addIncoming]: PushFact: "
            << DToString(ESGNodeCompressor[PushNod].first)
            << ", PushSE: " << NToString(ESGNodeCompressor[PushNod].second)
            << "; CurrSource: " << NToString(ESGNodeCompressor[CSSrc].second)
            << ", CSSrcFact: " << DToString(ESGNodeCompressor[CSSrc].first)
            << "; RetSite: " << NToString(RetSite));

    auto [It, Inserted] = Incoming[PushNod].try_emplace(
        std::pair{CSSrc, std::move(RetSite)}, std::move(PushWeight));
    if (!Inserted) {
      It->second = SR->combine(std::move(It->second), std::move(PushWeight));
    }
  }

  bool addEndSummary(wpds::ESGNodeId Src, cl_t TgtFact, se_t TgtSE,
                     weight_t TgtWeight) {
    auto [It, Inserted] = EndSummary[Src].try_emplace(
        std::pair{std::move(TgtFact), std::move(TgtSE)}, std::move(TgtWeight));
    if (!Inserted) {
      auto Merged = SR->combine(It->second, TgtWeight);
      if (It->second != Merged) {
        It->second = std::move(Merged);
        Inserted = true;
      }
    }
    return Inserted;
  }

  void applyEndSummary(wpds::ESGNodeId PushNod, wpds::ESGNodeId CSSrc,
                       se_t RetSite, weight_t PushWeight) {
    const auto &Sum = EndSummary[PushNod];
    const auto &PushSE = ESGNodeCompressor[PushNod].second;
    for (const auto &[SumNode, SumWeight] : Sum) {
      auto &&[SumCL, SumSE] = SumNode;
      auto SumExtWeight = SR->extend(PushWeight, SumWeight);
      for (const auto &[PopCL, PopWeight] :
           RP->getPopRules(SumCL, SumSE, RetSite, PushSE)) {
        propagate({CSSrc, RetSite, PopCL}, SR->extend(SumExtWeight, PopWeight));
      }
    }
  }

  NonNullPtr<RuleProviderT> RP{};
  NonNullPtr<SemiRingT> SR{};

  llvm::SmallVector<std::pair<path_edge_t, weight_t>> WL;

  TypedVector<wpds::ESGNodeId,
              llvm::SmallDenseMap<std::pair<cl_t, se_t>, weight_t>>
      EndSummary;
};

} // namespace psr
