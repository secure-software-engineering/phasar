/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_IFDSPROBLEM_H
#define PHASAR_DATAFLOW_IFDSIDE_IFDSPROBLEM_H

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/IDEProblem.h"
#include "phasar/Domain/AnalysisDomain.h"

namespace psr {
template <typename AnalysisDomainTy, typename AnalysisDataTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IFDSProblem : public IDEProblem<WithBinaryValueDomain<AnalysisDomainTy>,
                                      AnalysisDataTy, Container> {
  using base_t = IDEProblem<WithBinaryValueDomain<AnalysisDomainTy>,
                            AnalysisDataTy, Container>;

  using typename base_t::l_t;

public:
  using typename base_t::d_t;
  using typename base_t::f_t;
  using typename base_t::n_t;

  using typename base_t::container_type;
  using typename base_t::FlowFunctionPtrType;
  using typename base_t::FlowFunctionType;

  using typename base_t::analysis_data_t;

  EdgeFunction<l_t> getNormalEdgeFunction(analysis_data_t & /*Analysis*/,
                                          n_t /*Curr*/, d_t /*CurrNode*/,
                                          n_t /*Succ*/,
                                          d_t /*SuccNode*/) const final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getCallEdgeFunction(analysis_data_t & /*Analysis*/,
                                        n_t /*CallSite*/, d_t /*SrcNode*/,
                                        f_t /*CalleeFun*/,
                                        d_t /*DestNode*/) const final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getReturnEdgeFunction(analysis_data_t & /*Analysis*/,
                                          n_t /*CallSite*/, f_t /*CalleeFun*/,
                                          n_t /*ExitInst*/, d_t /*ExitNode*/,
                                          n_t /*RetSite*/,
                                          d_t /*RetNode*/) const final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(analysis_data_t & /*Analysis*/, n_t /*CallSite*/,
                           d_t /*CallNode*/, n_t /*RetSite*/,
                           d_t /*RetSiteNode*/,
                           llvm::ArrayRef<f_t> /*Callees*/) const final {
    return EdgeIdentity<l_t>{};
  }

  EdgeFunction<l_t> getSummaryEdgeFunction(analysis_data_t & /*Analysis*/,
                                           n_t /*Curr*/, d_t /*CurrNode*/,
                                           n_t /*Succ*/,
                                           d_t /*SuccNode*/) const final {
    return nullptr;
  }
};
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_IFDSPROBLEM_H
