#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/EntryPointUtils.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IfdsIdeDomain.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/SemiRing.h"

#include <type_traits>

namespace psr {
template <IfdsAnalysisDomain AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IfdsIdeProblemMixin
    : protected FlowFunctionTemplates<typename AnalysisDomainTy::d_t,
                                      Container>,
      public AnalysisDomainTy,
      public DerivedJoinLattice<
          typename detail::ValueDomainAdder<AnalysisDomainTy>::l_t>,
      public DefaultSemiRing<
          typename detail::ValueDomainAdder<AnalysisDomainTy>::l_t> {
protected:
  using FFTemplates =
      FlowFunctionTemplates<typename AnalysisDomainTy::d_t, Container>;

public:
  using ProblemAnalysisDomain = detail::ValueDomainAdder<AnalysisDomainTy>;
  using typename FFTemplates::container_type;
  using typename FFTemplates::FlowFunctionPtrType;
  using typename ProblemAnalysisDomain::d_t;
  using typename ProblemAnalysisDomain::db_t;
  using typename ProblemAnalysisDomain::f_t;
  using typename ProblemAnalysisDomain::i_t;
  using typename ProblemAnalysisDomain::l_t;
  using typename ProblemAnalysisDomain::n_t;
  using typename ProblemAnalysisDomain::t_t;
  using typename ProblemAnalysisDomain::v_t;
  using EdgeFunctionType = EdgeFunction<l_t>;

  [[nodiscard]] constexpr ByConstRef<d_t> getZeroValue() const noexcept {
    return ZeroValue;
  }

  [[nodiscard]] constexpr bool
  isZeroValue(ByConstRef<d_t> Fact) const noexcept {
    return Fact == ZeroValue;
  }

  [[nodiscard]] LLVM_ATTRIBUTE_RETURNS_NONNULL constexpr const auto *
  getProjectIRDB() const noexcept {
    return IRDB.get();
  }

  [[nodiscard]] constexpr const auto &getEntryPoints() const noexcept {
    return EntryPoints;
  }

  [[nodiscard]] FlowFunctionPtrType getSummaryFlowFunction(n_t /*CallSite*/,
                                                           f_t /*DestFun*/) {
    return nullptr;
  }

  EdgeFunctionType getSummaryEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                          n_t /*Succ*/, d_t /*SuccNode*/) {
    return EdgeIdentity<l_t>{};
  }

protected:
  constexpr IfdsIdeProblemMixin(NonNullPtr<const db_t> IRDB,
                                std::vector<std::string> EntryPoints,
                                d_t ZeroValue)
      : IRDB(IRDB), ZeroValue(std::move(ZeroValue)),
        EntryPoints(std::move(EntryPoints)) {}

  typename FlowFunctions<AnalysisDomainTy, container_type>::FlowFunctionPtrType
  generateFromZero(d_t FactToGenerate) {
    return FFTemplates::generateFlow(std::move(FactToGenerate), getZeroValue());
  }

  /// Seeds that just start with ZeroValue and bottomElement() at the starting
  /// points of each EntryPoint function.
  /// Takes the __ALL__ EntryPoint into account.
  [[nodiscard]] static InitialSeeds<n_t, d_t, l_t>
  createDefaultSeeds(auto &&Self)
    requires std::is_nothrow_default_constructible_v<
        typename AnalysisDomainTy::c_t>
  {
    static_assert(std::derived_from<std::remove_cvref_t<decltype(Self)>,
                                    IfdsIdeProblemMixin>);
    InitialSeeds<n_t, d_t, l_t> Seeds;
    typename AnalysisDomainTy::c_t C{};

    addSeedsForStartingPoints(Self.getEntryPoints(), Self.getProjectIRDB(), C,
                              Seeds, Self.getZeroValue(), Self.bottomElement());

    return Seeds;
  }

  NonNullPtr<const db_t> IRDB{};
  d_t ZeroValue{};
  std::vector<std::string> EntryPoints;
};
} // namespace psr
