#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/DataFlow/IfdsIde/IFDSProblem.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/NonNullPtr.h"

#include "llvm/ADT/ArrayRef.h"

namespace psr {

/// \brief A simple wrapper over a pointer to an IFDS problem.
///
template <IFDSProblem ProblemTy> class IFDSProblemWrapper {
public:
  using ProblemAnalysisDomain = typename ProblemTy::ProblemAnalysisDomain;
  using d_t = typename ProblemAnalysisDomain::d_t;
  using n_t = typename ProblemAnalysisDomain::n_t;
  using f_t = typename ProblemAnalysisDomain::f_t;
  using t_t = typename ProblemAnalysisDomain::t_t;
  using v_t = typename ProblemAnalysisDomain::v_t;
  using i_t = typename ProblemAnalysisDomain::i_t;
  using db_t = typename ProblemAnalysisDomain::db_t;

  constexpr IFDSProblemWrapper(NonNullPtr<ProblemTy> Problem) noexcept
      : Problem(Problem) {}
  constexpr IFDSProblemWrapper(ProblemTy *Problem) noexcept
      : Problem(Problem) {}

  // --- IFDSProblem:

  [[nodiscard]] constexpr bool isZeroValue(ByConstRef<d_t> Fact) const {
    if constexpr (requires { Problem->isZeroValue(Fact); }) {
      return Problem->isZeroValue(Fact);
    } else {
      return Fact == getZeroValue();
    }
  }

  [[nodiscard]] constexpr decltype(auto) getZeroValue() const {
    return Problem->getZeroValue();
  }

  [[nodiscard]] constexpr decltype(auto) initialSeeds() {
    return Problem->initialSeeds();
  }

  [[nodiscard]] constexpr ProjectIRDBConstPtr auto getProjectIRDB() const {
    return Problem->getProjectIRDB();
  }

  [[nodiscard]] constexpr decltype(auto) getEntryPoints() const {
    return Problem->getEntryPoints();
  }

  // --- FlowFunctionFactory:

  using container_type = typename ProblemTy::container_type;
  using FlowFunctionPtrType = typename ProblemTy::FlowFunctionPtrType;

  [[nodiscard]] constexpr decltype(auto)
  getNormalFlowFunction(ByConstRef<n_t> Curr, ByConstRef<n_t> Succ) {
    return Problem->getNormalFlowFunction(Curr, Succ);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallFlowFunction(ByConstRef<n_t> Curr, ByConstRef<f_t> CalleeFun) {
    return Problem->getCallFlowFunction(Curr, CalleeFun);
  }

  [[nodiscard]] constexpr decltype(auto)
  getRetFlowFunction(ByConstRef<n_t> CallSite, ByConstRef<f_t> CalleeFun,
                     ByConstRef<n_t> ExitInst, ByConstRef<n_t> RetSite) {
    return Problem->getRetFlowFunction(CallSite, CalleeFun, ExitInst, RetSite);
  }

  [[nodiscard]] constexpr decltype(auto)
  getCallToRetFlowFunction(ByConstRef<n_t> CallSite, ByConstRef<n_t> RetSite,
                           llvm::ArrayRef<f_t> Callees) {
    return Problem->getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  [[nodiscard]] constexpr decltype(auto)
  getSummaryFlowFunction(ByConstRef<n_t> Curr, ByConstRef<f_t> CalleeFun) {
    if constexpr (requires {
                    Problem->getSummaryFlowFunction(Curr, CalleeFun);
                  }) {
      return Problem->getSummaryFlowFunction(Curr, CalleeFun);
    } else {
      return nullptr;
    }
  }

  // ---

  /// The wrapped problem
  [[nodiscard]] auto &base() noexcept { return *Problem; }
  [[nodiscard]] const auto &base() const noexcept { return *Problem; }

protected:
  NonNullPtr<ProblemTy> Problem;
};

} // namespace psr
