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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFUNCTIONS_H_

#include <memory>
#include <set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

namespace psr {

template <typename N, typename D, typename F> class FlowFunctions {
public:
  virtual ~FlowFunctions() = default;
  virtual std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr,
                                                                 N succ) = 0;
  virtual std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                               F destFun) = 0;
  virtual std::shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, F calleeFun, N exitStmt, N retSite) = 0;
  virtual std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite, std::set<F> callees) = 0;
  virtual std::shared_ptr<FlowFunction<D>>
  getSummaryFlowFunction(N curr, F destFun) = 0;
};
} // namespace  psr

#endif
