#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/ControlFlow/SparseCFGProvider.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCache.h"
#include "phasar/DataFlow/WPDS/RuleProvider.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/SmallVector.h"

#include <type_traits>

namespace psr::wpds {

namespace detail {
template <bool ComputeWeights, typename ProblemT> struct WeightTypeOf {
  using type = EdgeIdentity<BinaryDomain>;
};
template <typename ProblemT> struct WeightTypeOf<true, ProblemT> {
  using type = typename ProblemT::EdgeFunctionType;
};
} // namespace detail

template <typename ProblemT, typename ICFGTy, bool ComputeWeights>
class IfdsIdeRuleProvider {
public:
  using control_location_type = typename ProblemT::d_t;
  using stack_element_type = typename ProblemT::n_t;
  using weight_type =
      typename detail::WeightTypeOf<ComputeWeights, ProblemT>::type;

  static constexpr auto LogCategory =
      ComputeWeights ? llvm::StringLiteral("IDERuleProvider")
                     : llvm::StringLiteral("IFDSRuleProvider");

  IfdsIdeRuleProvider(
      ProblemT *Problem, const ICFGTy *ICF,
      std::bool_constant<ComputeWeights> /*unused*/ = {}) noexcept
      : Problem(&assertNotNull(Problem)), ICF(&assertNotNull(ICF)) {
    static_assert(RuleProvider<IfdsIdeRuleProvider>);
  }

  [[nodiscard]] auto getNormalRules(ByConstRef<control_location_type> CL,
                                    ByConstRef<stack_element_type> SE) {
    llvm::SmallVector<
        std::tuple<control_location_type, stack_element_type, weight_type>>
        Outs;

    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "[getNormalRules]: CL=" << DToString(CL)
                                                 << "; SE=" << NToString(SE));

    if (ICF->isCallSite(SE)) {
      auto Callees = ICF->getCalleesOfCallAt(SE);
      auto RetSites = ICF->getReturnSitesOfCallAt(SE);
      for (const auto &Succ : RetSites) {
        auto Facts = FECache.getCallToRetFlowFunction(SE, Succ, Callees)
                         ->computeTargets(CL);
        for (auto &Fct : Facts) {
          const auto FctSucc = factSucc(Succ, Fct);
          if constexpr (ComputeWeights) {
            auto W =
                FECache.getCallToRetEdgeFunction(SE, CL, Succ, Fct, Callees);
            Outs.emplace_back(std::move(Fct), FctSucc, std::move(W));
          } else {
            Outs.emplace_back(std::move(Fct), FctSucc, weight_type{});
          }
        }
      }

      for (const auto &DestFun : Callees) {
        if (auto SumFF = FECache.getSummaryFlowFunction(SE, DestFun)) {
          auto Facts = SumFF->computeTargets(CL);
          for (const auto &Succ : RetSites) {
            for (auto &Fct : Facts) {
              const auto FctSucc = factSucc(Succ, Fct);
              if constexpr (ComputeWeights) {
                auto W = FECache.getSummaryEdgeFunction(SE, CL, Succ, Fct);
                Outs.emplace_back(Fct, FctSucc, std::move(W));
              } else {
                Outs.emplace_back(Fct, FctSucc, weight_type{});
              }
            }
          }
        }
      }

    } else {
      for (const auto &Succ : ICF->getSuccsOf(SE)) {
        auto Facts =
            FECache.getNormalFlowFunction(SE, Succ)->computeTargets(CL);
        for (auto &Fct : Facts) {
          const auto FctSucc = factSucc(Succ, Fct);
          if constexpr (ComputeWeights) {
            auto W = FECache.getNormalEdgeFunction(SE, CL, Succ, Fct);
            Outs.emplace_back(std::move(Fct), FctSucc, std::move(W));
          } else {
            Outs.emplace_back(std::move(Fct), FctSucc, weight_type{});
          }
        }
      }
    }

    return Outs;
  }

  [[nodiscard]] auto getPushRules(ByConstRef<control_location_type> CL,
                                  ByConstRef<stack_element_type> SE) {
    llvm::SmallVector<std::tuple<control_location_type, stack_element_type,
                                 stack_element_type, weight_type>>
        Outs;
    if (!ICF->isCallSite(SE)) {
      return Outs;
    }

    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "[getPushRules]: CL=" << DToString(CL)
                                               << "; SE=" << NToString(SE));

    auto Callees = ICF->getCalleesOfCallAt(SE);
    auto RetSites = ICF->getReturnSitesOfCallAt(SE);
    for (const auto &DestFun : Callees) {
      if (FECache.getSummaryFlowFunction(SE, DestFun)) {
        // Handled in getNormalRules()
        continue;
      }

      auto Facts = FECache.getCallFlowFunction(SE, DestFun)->computeTargets(CL);
      auto EntrySEs = ICF->getStartPointsOf(DestFun);
      for (auto &&Fct : Facts) {
        auto W = [&] {
          if constexpr (ComputeWeights) {
            return FECache.getCallEdgeFunction(SE, CL, DestFun, Fct);
          } else {
            return weight_type{};
          }
        }();
        for (const auto &Succ : RetSites) {
          const auto FctSucc = factSucc(Succ, Fct);
          for (const auto &EntrySE : EntrySEs) {
            Outs.emplace_back(Fct, FctSucc, EntrySE, W);
          }
        }
      }
    }

    return Outs;
  }

  [[nodiscard]] bool hasPopRules(ByConstRef<control_location_type> /*CL*/,
                                 ByConstRef<stack_element_type> SE) {
    // TODO: Be more precise here, filtering for facts CL that we actually need
    // in the summary.
    return ICF->isExitInst(SE);
  }

  [[nodiscard]] auto getPopRules(ByConstRef<control_location_type> CL,
                                 ByConstRef<stack_element_type> ExitSE,
                                 ByConstRef<stack_element_type> RetSiteSE,
                                 ByConstRef<stack_element_type> /*EntrySE*/
  ) {
    llvm::SmallVector<std::tuple<control_location_type, weight_type>> Outs;

    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "[getPopRules]: CL="
                             << DToString(CL)
                             << "; ExitSE=" << NToString(ExitSE)
                             << "; RetSiteSE=" << NToString(RetSiteSE));

    auto DestFun = ICF->getFunctionOf(ExitSE);

    for (const auto &CS : ICF->getPredsOf(RetSiteSE)) {
      if (!ICF->isCallSite(CS)) {
        continue;
      }

      auto Facts = FECache.getRetFlowFunction(CS, DestFun, ExitSE, RetSiteSE)
                       ->computeTargets(CL);
      for (auto &Fct : Facts) {
        if constexpr (ComputeWeights) {
          auto W = FECache.getReturnEdgeFunction(CS, DestFun, ExitSE, CL,
                                                 RetSiteSE, Fct);
          Outs.emplace_back(std::move(Fct), std::move(W));
        } else {
          Outs.emplace_back(std::move(Fct), weight_type{});
        }
      }
    }

    return Outs;
  }

  [[nodiscard]] auto initialSeeds() {
    llvm::SmallVector<
        std::tuple<control_location_type, stack_element_type, weight_type>>
        Outs;

    for (const auto &[Inst, Facts] : Problem->initialSeeds().getSeeds()) {
      for (const auto &[Fact, Val] : Facts) {
        if constexpr (ComputeWeights) {
          using l_t = typename ProblemT::l_t;
          if (Val == Problem->topElement()) {
            continue;
          }
          if (Val == Problem->bottomElement()) {
            if constexpr (HasJoinLatticeTraits<l_t>) {
              Outs.emplace_back(Fact, Inst, AllBottom<l_t>{});
            } else {
              Outs.emplace_back(Fact, Inst, AllBottom<l_t>{Val});
            }
          } else {
            Outs.emplace_back(
                Fact, Inst,
                ConstantEdgeFunction<l_t>{NonTopBotValue<l_t>::unwrap(Val)});
          }
        } else {
          Outs.emplace_back(Fact, Inst, weight_type{});
        }
      }
    }

    return Outs;
  }

  [[nodiscard]] constexpr auto &problem() const noexcept { return *Problem; }

private:
  [[nodiscard]] auto factSucc(stack_element_type Succ,
                              ByConstRef<control_location_type> CL) {
    if constexpr (has_advanceToNextUser_v<ICFGTy, control_location_type>) {
      return ICF->advancetoNextUser(Succ, CL);
    } else {
      return Succ;
    }
  }

  ProblemT *Problem{};
  const ICFGTy *ICF{};

  FlowEdgeFunctionCache<typename ProblemT::ProblemAnalysisDomain> FECache{
      *Problem};
};

template <typename ProblemT, typename ICFGTy>
using IDERuleProvider = IfdsIdeRuleProvider<ProblemT, ICFGTy, true>;

template <typename ProblemT, typename ICFGTy>
using IFDSRuleProvider = IfdsIdeRuleProvider<ProblemT, ICFGTy, false>;

} // namespace psr::wpds
