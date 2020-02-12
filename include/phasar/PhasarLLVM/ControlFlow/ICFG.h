/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ICFG.h
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_ICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_ICFG_H_

#include <iostream>
#include <map>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

#include <phasar/PhasarLLVM/ControlFlow/CFG.h>

namespace psr {

enum class CallGraphAnalysisType {
#define ANALYSIS_SETUP_CALLGRAPH_TYPE(NAME, CMDFLAG, TYPE) TYPE,
#include <phasar/PhasarLLVM/Utils/AnalysisSetups.def>
  Invalid
};

std::string to_string(const CallGraphAnalysisType &CGA);

CallGraphAnalysisType to_CallGraphAnalysisType(const std::string &S);

std::ostream &operator<<(std::ostream &os, const CallGraphAnalysisType &CGA);

template <typename N, typename F> class ICFG : public virtual CFG<N, F> {
public:
  ~ICFG() override = default;

  virtual std::set<F> getAllFunctions() const = 0;

  virtual F getFunction(const std::string &fun) const = 0;

  virtual bool isCallStmt(N stmt) const = 0;

  virtual bool isIndirectFunctionCall(N stmt) const = 0;

  virtual bool isVirtualFunctionCall(N stmt) const = 0;

  virtual std::set<N> allNonCallStartNodes() const = 0;

  virtual std::set<F> getCalleesOfCallAt(N stmt) const = 0;

  virtual std::set<N> getCallersOf(F fun) const = 0;

  virtual std::set<N> getCallsFromWithin(F fun) const = 0;

  virtual std::set<N> getStartPointsOf(F fun) const = 0;

  virtual std::set<N> getExitPointsOf(F fun) const = 0;

  virtual std::set<N> getReturnSitesOfCallAt(N stmt) const = 0;

  using CFG<N, F>::print; // tell the compiler we wish to have both prints
  virtual void print(std::ostream &OS = std::cout) const = 0;

  using CFG<N, F>::getAsJson; // tell the compiler we wish to have both prints
  virtual nlohmann::json getAsJson() const = 0;
};

} // namespace psr

#endif
