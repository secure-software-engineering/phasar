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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

#include <memory>
#include <set>
#include <type_traits>

namespace psr {

template <typename N, typename D, typename F, typename Container = std::set<D>>
class FlowFunctions {
  static_assert(
      std::is_same<typename Container::value_type, D>::value,
      "Value type of the container needs to match the type parameter D");

public:
  using FlowFunctionType = std::shared_ptr<FlowFunction<D, Container>>;

  virtual ~FlowFunctions() = default;
  virtual FlowFunctionType getNormalFlowFunction(N curr, N succ) = 0;
  virtual FlowFunctionType getCallFlowFunction(N callStmt, F destFun) = 0;
  virtual FlowFunctionType getRetFlowFunction(N callSite, F calleeFun,
                                              N exitStmt, N retSite) = 0;
  virtual FlowFunctionType getCallToRetFlowFunction(N callSite, N retSite,
                                                    std::set<F> callees) = 0;
  virtual FlowFunctionType getSummaryFlowFunction(N curr, F destFun) = 0;
};
} // namespace  psr

#endif
