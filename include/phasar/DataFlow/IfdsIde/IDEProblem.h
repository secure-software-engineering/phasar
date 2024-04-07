/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_IDEPROBLEM_H
#define PHASAR_DATAFLOW_IFDSIDE_IDEPROBLEM_H

#include "phasar/DataFlow/IfdsIde/AllTopFnProvider.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/Utils/JoinLattice.h"

#include <set>

namespace psr {
template <typename AnalysisDomainTy, typename AnalysisDataTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDEProblem
    : public JoinLatticeProvider<AnalysisDomainTy, AnalysisDataTy>,
      public AllTopFnProvider<AnalysisDomainTy>,
      protected FlowFunctionTemplates<typename AnalysisDomainTy::d_t,
                                      Container> {
public:
  using d_t = typename AnalysisDomainTy::d_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using l_t = typename AnalysisDomainTy::l_t;

  using FlowFunctionType = FlowFunction<d_t, Container>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  using container_type = typename FlowFunctionType::container_type;

  using analysis_data_t = AnalysisDataTy;

  virtual ~IDEProblem() = default;

  ////////////////////////////////////////////////////////////////////////////////
  // Flow Function Factories
  //

  virtual FlowFunctionPtrType getNormalFlowFunction(analysis_data_t &Analysis,
                                                    n_t Curr,
                                                    n_t Succ) const = 0;

  virtual FlowFunctionPtrType getCallFlowFunction(analysis_data_t &Analysis,
                                                  n_t CallSite,
                                                  f_t CalleeFun) = 0;

  virtual FlowFunctionPtrType getRetFlowFunction(analysis_data_t &Analysis,
                                                 n_t CallSite, f_t CalleeFun,
                                                 n_t ExitInst,
                                                 n_t RetSite) const = 0;

  virtual void
  applyUnbalancedRetFlowFunctionSideEffects(analysis_data_t & /*Analysis*/,
                                            f_t /*CalleeFun*/, n_t /*ExitInst*/,
                                            d_t /*Source*/) const {
    // By default, do nothing
  }

  virtual FlowFunctionPtrType
  getCallToRetFlowFunction(analysis_data_t &Analysis, n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) const = 0;

  virtual FlowFunctionPtrType
  getSummaryFlowFunction(analysis_data_t & /*Analysis*/, n_t /*Curr*/,
                         f_t /*CalleeFun*/) const {
    return nullptr;
  }

  //
  ////////////////////////////////////////////////////////////////////////////////
  // Edge Function Factories
  //

  virtual EdgeFunction<l_t> getNormalEdgeFunction(analysis_data_t &Analysis,
                                                  n_t Curr, d_t CurrNode,
                                                  n_t Succ,
                                                  d_t SuccNode) const = 0;

  virtual EdgeFunction<l_t> getCallEdgeFunction(analysis_data_t &Analysis,
                                                n_t CallSite, d_t SrcNode,
                                                f_t CalleeFun,
                                                d_t DestNode) const = 0;

  virtual EdgeFunction<l_t> getReturnEdgeFunction(analysis_data_t &Analysis,
                                                  n_t CallSite, f_t CalleeFun,
                                                  n_t ExitInst, d_t ExitNode,
                                                  n_t RetSite,
                                                  d_t RetNode) const = 0;

  virtual EdgeFunction<l_t>
  getCallToRetEdgeFunction(analysis_data_t &Analysis, n_t CallSite,
                           d_t CallNode, n_t RetSite, d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) const = 0;

  virtual EdgeFunction<l_t>
  getSummaryEdgeFunction(analysis_data_t & /*Analysis*/, n_t /*Curr*/,
                         d_t /*CurrNode*/, n_t /*Succ*/,
                         d_t /*SuccNode*/) const {
    return nullptr;
  }

  //
  ////////////////////////////////////////////////////////////////////////////////
  // EdgeFunction Semi-Ring
  //

  virtual EdgeFunction<l_t> extend(analysis_data_t & /*Analysis*/,
                                   const EdgeFunction<l_t> &FirstEF,
                                   const EdgeFunction<l_t> &SecondEF) const {
    return FirstEF.composeWith(SecondEF);
  }
  virtual EdgeFunction<l_t> combine(analysis_data_t & /*Analysis*/,
                                    const EdgeFunction<l_t> &FirstEF,
                                    const EdgeFunction<l_t> &OtherEF) const {
    return FirstEF.joinWith(OtherEF);
  }

  //
  ////////////////////////////////////////////////////////////////////////////////
  // Misc Functions
  //

  /// Returns initial seeds to be used for the analysis. This is a mapping of
  /// statements to initial analysis facts.
  [[nodiscard]] virtual InitialSeeds<n_t, d_t, l_t>
  initialSeeds(analysis_data_t &Analysis) const = 0;

  /// Returns the special tautological lambda (or zero) fact.
  [[nodiscard]] virtual ByConstRef<d_t>
  getZeroValue(analysis_data_t &Analysis) const = 0;

protected:
  typename FlowFunctions<AnalysisDomainTy, Container>::FlowFunctionPtrType
  generateFromZero(d_t FactToGenerate) const {
    return FlowFunctions<AnalysisDomainTy, Container>::generateFlow(
        std::move(FactToGenerate), getZeroValue());
  }
};
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_IDEPROBLEM_H
