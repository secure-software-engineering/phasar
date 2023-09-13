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
#include "phasar/Domain/BinaryDomain.h"

#include <set>
#include <string>

namespace psr {

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

  explicit IFDSTabulationProblem(const ProjectIRDBBase<db_t> *IRDB,
                                 std::vector<std::string> EntryPoints,
                                 d_t ZeroValue)
      : Base(IRDB, std::move(EntryPoints), std::move(ZeroValue)) {}
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
