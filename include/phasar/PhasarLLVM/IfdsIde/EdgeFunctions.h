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

#ifndef ANALYSIS_IFDS_IDE_EDGEFUNCTIONS_H_
#define ANALYSIS_IFDS_IDE_EDGEFUNCTIONS_H_

#include "EdgeFunction.h"
#include <memory>

using namespace std;
namespace psr {

template <typename N, typename D, typename M, typename V> class EdgeFunctions {
public:
  virtual ~EdgeFunctions() = default;
  virtual shared_ptr<EdgeFunction<V>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) = 0;
  virtual shared_ptr<EdgeFunction<V>> getCallEdgeFunction(N callStmt, D srcNode,
                                                          M destiantionMethod,
                                                          D destNode) = 0;
  virtual shared_ptr<EdgeFunction<V>>
  getReturnEdgeFunction(N callSite, M calleeMethod, N exitStmt, D exitNode,
                        N reSite, D retNode) = 0;
  virtual shared_ptr<EdgeFunction<V>>
  getCallToReturnEdgeFunction(N callSite, D callNode, N retSite,
                              D retSiteNode) = 0;
  virtual shared_ptr<EdgeFunction<V>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) = 0;
};

} // namespace psr

#endif /* ANALYSIS_IFDS_IDE_EDGEFUNCTIONS_HH_ */
