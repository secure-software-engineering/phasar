#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/SemiRing.h"
#include "phasar/Utils/Utilities.h"

namespace psr {
template <typename AnalysisDomainTy>
class IfdsIdeProblemMixin
    : protected FlowFunctionTemplates<typename AnalysisDomainTy::d_t,
                                      std::set<typename AnalysisDomainTy::d_t>>,
      public AnalysisDomainTy,
      public DerivedJoinLattice<
          typename detail::ValueDomainAdder<AnalysisDomainTy>::l_t>,
      public DefaultSemiRing<
          typename detail::ValueDomainAdder<AnalysisDomainTy>::l_t> {
protected:
  using FFTemplates =
      FlowFunctionTemplates<typename AnalysisDomainTy::d_t,
                            std::set<typename AnalysisDomainTy::d_t>>;

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

  [[nodiscard]] constexpr auto getZeroValue() const noexcept {
    assert(ZeroValue);
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

protected:
  constexpr IfdsIdeProblemMixin(NonNullPtr<const db_t> IRDB,
                                std::vector<std::string> EntryPoints,
                                d_t ZeroValue)
      : IRDB(IRDB), ZeroValue(std::move(ZeroValue)),
        EntryPoints(std::move(EntryPoints)) {}

  NonNullPtr<const db_t> IRDB{};
  Nullable<d_t> ZeroValue{};
  std::vector<std::string> EntryPoints;
};
} // namespace psr
