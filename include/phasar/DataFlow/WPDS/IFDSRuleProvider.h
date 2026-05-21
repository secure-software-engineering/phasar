#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/Solver/FlowEdgeFunctionCache.h"
#include "phasar/DataFlow/WPDS/RuleProvider.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/SmallVector.h"

namespace psr::wpds {
template <typename ProblemT, typename ICFGTy> class IFDSRuleProvider {
  using l_t = BinaryDomain;

public:
  using control_location_type = typename ProblemT::d_t;
  using stack_element_type = typename ProblemT::n_t;
  using weight_type = EdgeIdentity<l_t>;

  static constexpr llvm::StringLiteral LogCategory = "IDERuleProvider";

  IFDSRuleProvider(ProblemT *Problem, const ICFGTy *ICF) noexcept
      : Problem(&assertNotNull(Problem)), ICF(&assertNotNull(ICF)) {
    static_assert(RuleProvider<IFDSRuleProvider>);
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
          Outs.emplace_back(std::move(Fct), Succ, weight_type{});
        }
      }

      for (const auto &DestFun : Callees) {
        if (auto SumFF = FECache.getSummaryFlowFunction(SE, DestFun)) {
          auto Facts = SumFF->computeTargets(CL);
          for (const auto &Succ : RetSites) {
            for (auto &Fct : Facts) {
              Outs.emplace_back(Fct, Succ, weight_type{});
            }
          }
        }
      }

    } else {
      for (const auto &Succ : ICF->getSuccsOf(SE)) {
        auto Facts =
            FECache.getNormalFlowFunction(SE, Succ)->computeTargets(CL);
        for (auto &Fct : Facts) {
          Outs.emplace_back(std::move(Fct), Succ, weight_type{});
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
      for (const auto &EntrySE : EntrySEs) {
        for (const auto &Succ : RetSites) {
          for (auto &&Fct : Facts) {
            Outs.emplace_back(Fct, Succ, EntrySE, weight_type{});
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
        Outs.emplace_back(std::move(Fct), weight_type{});
      }
    }

    return Outs;
  }

  [[nodiscard]] auto initialSeeds() {
    llvm::SmallVector<
        std::tuple<control_location_type, stack_element_type, weight_type>>
        Outs;

    for (const auto &[Inst, Facts] : Problem->initialSeeds().getSeeds()) {
      for (const auto &[Fact, _] : Facts) {
        Outs.emplace_back(Fact, Inst, weight_type{});
      }
    }

    return Outs;
  }

  [[nodiscard]] constexpr auto &ifdsProblem() const noexcept {
    return *Problem;
  }

private:
  ProblemT *Problem{};
  const ICFGTy *ICF{};

  FlowEdgeFunctionCache<typename ProblemT::ProblemAnalysisDomain> FECache{
      *Problem};
};
} // namespace psr::wpds
