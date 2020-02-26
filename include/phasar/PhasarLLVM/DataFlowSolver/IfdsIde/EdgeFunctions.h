/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AbstractEdgeFunctions.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_H_

#include <memory>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <set>

namespace psr {

template <typename N, typename D, typename F, typename L> class EdgeFunctions {
public:
  virtual ~EdgeFunctions() = default;
  virtual std::shared_ptr<EdgeFunction<L>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) = 0;
  virtual std::shared_ptr<EdgeFunction<L>>
  getCallEdgeFunction(N callStmt, D srcNode, F destinationFunction,
                      D destNode) = 0;
  virtual std::shared_ptr<EdgeFunction<L>>
  getReturnEdgeFunction(N callSite, F calleeFunction, N exitStmt, D exitNode,
                        N reSite, D retNode) = 0;
  virtual std::shared_ptr<EdgeFunction<L>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<F> callees) = 0;
  virtual std::shared_ptr<EdgeFunction<L>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) = 0;
};

} // namespace psr

#endif
