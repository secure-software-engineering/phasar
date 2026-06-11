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
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/IDEProblem.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/NonNullPtr.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"

#include <type_traits>

namespace psr {

/// \brief A simple wrapper over a pointer to an IFDS/IDE problem, implementing
/// the legacy IDETabulationProblem
template <IDEProblem ProblemTy>
class LegacyIDEProblemWrapper
    : public IDETabulationProblem<typename ProblemTy::ProblemAnalysisDomain,
                                  typename ProblemTy::container_type> {
  using base_t = IDETabulationProblem<typename ProblemTy::ProblemAnalysisDomain,
                                      typename ProblemTy::container_type>;
  static std::vector<std::string> makeEntryVec(auto &&Range) {
    if constexpr (std::is_convertible_v<decltype(Range),
                                        std::vector<std::string>>) {
      return PSR_FWD(Range);
    } else {
      return std::vector<std::string>(llvm::adl_begin(Range),
                                      llvm::adl_end(Range));
    }
  }

public:
  using ProblemAnalysisDomain = typename ProblemTy::ProblemAnalysisDomain;
  using d_t = typename ProblemAnalysisDomain::d_t;
  using n_t = typename ProblemAnalysisDomain::n_t;
  using f_t = typename ProblemAnalysisDomain::f_t;
  using t_t = typename ProblemAnalysisDomain::t_t;
  using v_t = typename ProblemAnalysisDomain::v_t;
  using l_t = typename ProblemAnalysisDomain::l_t;
  using i_t = typename ProblemAnalysisDomain::i_t;
  using db_t = typename ProblemAnalysisDomain::db_t;

  LegacyIDEProblemWrapper(NonNullPtr<ProblemTy> Problem) noexcept
      : base_t(Problem->getProjectIRDB(),
               makeEntryVec(Problem->getEntryPoints()),
               Problem->getZeroValue()),
        Problem(Problem) {}

  LegacyIDEProblemWrapper(ProblemTy *Problem) noexcept
      : LegacyIDEProblemWrapper(NonNullPtr<ProblemTy>(Problem)) {}

  // --- IFDSProblem:

  [[nodiscard]] bool isZeroValue(d_t Fact) const final {
    if constexpr (requires { Problem->isZeroValue(Fact); }) {
      return Problem->isZeroValue(Fact);
    } else {
      return this->base_t::getZeroValue() == Fact;
    }
  }

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() final {
    return Problem->initialSeeds();
  }

  void emitTextReport(GenericSolverResults<n_t, d_t, l_t> Results,
                      llvm::raw_ostream &OS = llvm::outs()) final {
    if constexpr (requires { Problem->emitTextReport(Results, OS); }) {
      Problem->emitTextReport(Results, OS);
    } else {
      this->base_t::emitTextReport(Results, OS);
    }
  }

  void emitGraphicalReport(GenericSolverResults<n_t, d_t, l_t> Results,
                           llvm::raw_ostream &OS = llvm::outs()) final {
    if constexpr (requires { Problem->emitGraphicalReport(Results, OS); }) {
      Problem->emitGraphicalReport(Results, OS);
    } else {
      this->base_t::emitGraphicalReport(Results, OS);
    }
  }

  bool setSoundness(Soundness S) final {
    if constexpr (requires { Problem->setSoundness(S); }) {
      return Problem->setSoundness(S);
    } else {
      return false;
    }
  }

  // --- FlowFunctionFactory:

  using container_type = typename ProblemTy::container_type;
  using FlowFunctionPtrType = typename ProblemTy::FlowFunctionPtrType;

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunction(n_t Curr,
                                                          n_t Succ) final {
    return Problem->getNormalFlowFunction(Curr, Succ);
  }

  [[nodiscard]] FlowFunctionPtrType getCallFlowFunction(n_t Curr,
                                                        f_t CalleeFun) final {
    return Problem->getCallFlowFunction(Curr, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType getRetFlowFunction(n_t CallSite,
                                                       f_t CalleeFun,
                                                       n_t ExitInst,
                                                       n_t RetSite) final {
    return Problem->getRetFlowFunction(CallSite, CalleeFun, ExitInst, RetSite);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) final {
    return Problem->getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  [[nodiscard]] FlowFunctionPtrType
  getSummaryFlowFunction(n_t Curr, f_t CalleeFun) final {
    if constexpr (requires {
                    Problem->getSummaryFlowFunction(Curr, CalleeFun);
                  }) {
      return Problem->getSummaryFlowFunction(Curr, CalleeFun);
    } else {
      return nullptr;
    }
  }

  // --- EdgeFunctionFactory:

  using EdgeFunctionType = typename ProblemTy::EdgeFunctionType;

  [[nodiscard]] EdgeFunctionType
  getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ, d_t SuccNode) final {
    return Problem->getNormalEdgeFunction(Curr, CurrNode, Succ, SuccNode);
  }

  [[nodiscard]] EdgeFunctionType getCallEdgeFunction(n_t CallSite, d_t CSNode,
                                                     f_t CalleeFun,
                                                     d_t CalleeNode) final {
    return Problem->getCallEdgeFunction(CallSite, CSNode, CalleeFun,
                                        CalleeNode);
  }

  [[nodiscard]] EdgeFunctionType
  getReturnEdgeFunction(n_t CallSite, f_t CalleeFun, n_t ExitInst, d_t ExitNode,
                        n_t RetSite, d_t RSNode) final {
    return Problem->getReturnEdgeFunction(CallSite, CalleeFun, ExitInst,
                                          ExitNode, RetSite, RSNode);
  }

  [[nodiscard]] EdgeFunctionType
  getCallToRetEdgeFunction(n_t CallSite, d_t CSNode, n_t RetSite, d_t RSNode,
                           llvm::ArrayRef<f_t> Callees) final {
    return Problem->getCallToRetEdgeFunction(CallSite, CSNode, RetSite, RSNode,
                                             Callees);
  }

  [[nodiscard]] EdgeFunctionType
  getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ, d_t SuccNode) final {
    if constexpr (requires {
                    Problem->getSummaryEdgeFunction(Curr, CurrNode, Succ,
                                                    SuccNode);
                  }) {

      return Problem->getSummaryEdgeFunction(Curr, CurrNode, Succ, SuccNode);
    } else {
      return getDefaultValue<EdgeFunctionType>();
    }
  }

  // --- IsJoinLattice:

  [[nodiscard]] l_t topElement() final {
    if constexpr (requires { Problem->topElement(); }) {
      return Problem->topElement();
    } else {
      return JoinLatticeTraits<l_t>::top();
    }
  }

  [[nodiscard]] l_t bottomElement() final {
    if constexpr (requires { Problem->bottomElement(); }) {
      return Problem->bottomElement();
    } else {
      return JoinLatticeTraits<l_t>::bottom();
    }
  }

  [[nodiscard]] l_t join(l_t L, l_t R) final {
    if constexpr (requires(l_t Val) { Problem->join(Val, Val); }) {
      return Problem->join(std::move(L), std::move(R));
    } else {
      return JoinLatticeTraits<l_t>::join(std::move(L), std::move(R));
    }
  }

  // --- IsSemiRing:

  [[nodiscard]] EdgeFunctionType extend(const EdgeFunctionType &First,
                                        const EdgeFunctionType &Second) final {
    return Problem->extend(First, Second);
  }

  [[nodiscard]] EdgeFunctionType combine(const EdgeFunctionType &First,
                                         const EdgeFunctionType &Second) final {
    return Problem->combine(First, Second);
  }

  [[nodiscard]] EdgeFunctionType identity() final {
    if constexpr (requires { Problem->identity(); }) {
      return Problem->identity();
    } else {
      return EdgeIdentity<l_t>{};
    }
  }

  [[nodiscard]] EdgeFunctionType allTopFunction() final {
    if constexpr (requires { Problem->allTopFunction(); }) {
      return Problem->allTopFunction();
    } else if constexpr (HasJoinLatticeTraits<l_t>) {
      return AllTop<l_t>{};
    } else {
      return AllTop<l_t>{topElement()};
    }
  }

protected:
  NonNullPtr<ProblemTy> Problem;
};

} // namespace psr
