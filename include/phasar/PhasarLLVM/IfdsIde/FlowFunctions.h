/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * FlowFunctions.h
 *
 *  Created on: 03.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_FLOWFUNCTIONS_H_
#define ANALYSIS_FLOWFUNCTIONS_H_

#include "FlowFunction.h"
#include <memory>
#include <vector>

using namespace std;
namespace psr{

template <typename N, typename D, typename M> class FlowFunctions {
public:
  virtual ~FlowFunctions() = default;
  virtual shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) = 0;
  virtual shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                          M destMthd) = 0;
  virtual shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, M calleeMthd, N exitStmt, N retSite) = 0;
  virtual shared_ptr<FlowFunction<D>> getCallToRetFlowFunction(N callSite,
                                                               N retSite) = 0;
  virtual shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N curr,
                                                             M destMthd) = 0;
};
}//namespace  psr

#endif /* ANALYSIS_ABSTRACTFLOWFUNCTIONS_HH_ */
