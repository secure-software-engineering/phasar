/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_IFDSTABULATIONPROBLEM_H
#define PHASAR_DATAFLOW_IFDSIDE_IFDSTABULATIONPROBLEM_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/Domain/AnalysisDomain.h"

#include <set>
#include <string>

namespace psr {

/// \brief The analysis problem interface for IFDS problems (solvable by the
/// IFDSSolver). Create a subclass from this and override all pure-virtual
/// functions to create your own IFDS analysis.
///
/// For more information on how to write an IFDS analysis, see [Writing an IFDS
/// Analysis](https://github.com/secure-software-engineering/phasar/wiki/Writing-an-IFDS-analysis)
template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IFDSTabulationProblem
    : public IDETabulationProblem<WithBinaryValueDomain<AnalysisDomainTy>,
                                  Container> {
  using Base =
      IDETabulationProblem<WithBinaryValueDomain<AnalysisDomainTy>, Container>;

public:
  using typename Base::d_t;
  using typename Base::db_t;
  using typename Base::f_t;
  using typename Base::i_t;
  using typename Base::l_t;
  using typename Base::n_t;
  using typename Base::ProblemAnalysisDomain;
  using typename Base::t_t;
  using typename Base::v_t;

  /// Takes an IR database (IRDB) and collects information from it to create a
  /// tabulation problem.
  /// @param[in] IRDB The project IR database, that holds the code under
  /// analysis
  /// @param[in] EntryPoints The (mangled) names of all entry functions of the
  /// target being analyzed, given as a vector of strings. An example would
  /// simply be `{"main"}`. To set every function as entry point, pass
  /// `"__ALL__"`
  /// @param[in] ZeroValue Provides the special tautological zero value (aka.
  /// Λ).
  /// \endlink.
  explicit IFDSTabulationProblem(const ProjectIRDBBase<db_t> *IRDB,
                                 std::vector<std::string> EntryPoints,
                                 d_t ZeroValue)
      : Base(IRDB, std::move(EntryPoints), std::move(ZeroValue)) {}

  IFDSTabulationProblem(IFDSTabulationProblem &&) noexcept = default;
  IFDSTabulationProblem &operator=(IFDSTabulationProblem &&) noexcept = default;

  IFDSTabulationProblem(const IFDSTabulationProblem &) = delete;
  IFDSTabulationProblem &operator=(const IFDSTabulationProblem &) = delete;

  ~IFDSTabulationProblem() override = default;

  EdgeFunction<l_t> getNormalEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                          n_t /*Succ*/,
                                          d_t /*SuccNode*/) final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getCallEdgeFunction(n_t /*CallInst*/, d_t /*SrcNode*/,
                                        f_t /*CalleeFun*/,
                                        d_t /*DestNode*/) final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getReturnEdgeFunction(n_t /*CallSite*/, f_t /*CalleeFun*/,
                                          n_t /*ExitInst*/, d_t /*ExitNode*/,
                                          n_t /*RetSite*/,
                                          d_t /*RetNode*/) final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/,
                           d_t /*RetSiteNode*/,
                           llvm::ArrayRef<f_t> /*Callees*/) final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t /*Curr*/, d_t /*CurrNode*/,
                                           n_t /*Succ*/,
                                           d_t /*SuccNode*/) final {
    return EdgeIdentity<l_t>{};
  }
};
} // namespace psr

#endif
