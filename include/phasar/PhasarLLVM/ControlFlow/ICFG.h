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

#include <iosfwd>
#include <map>
#include <set>
#include <string>

#include <json.hpp>

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

using json = nlohmann::json;

template <typename N, typename M> class ICFG : public virtual CFG<N, M> {
public:
  ~ICFG() override = default;

  virtual std::set<M> getAllFunctions() const = 0;

  virtual M getFunction(const std::string &fun) const = 0;

  virtual bool isCallStmt(N stmt) const = 0;

  virtual bool isIndirectFunctionCall(N stmt) const = 0;

  virtual bool isVirtualFunctionCall(N stmt) const = 0;

  virtual std::set<N> allNonCallStartNodes() const = 0;

  virtual std::set<M> getCalleesOfCallAt(N stmt) const = 0;

  virtual std::set<N> getCallersOf(M fun) const = 0;

  virtual std::set<N> getCallsFromWithin(M fun) const = 0;

  virtual std::set<N> getStartPointsOf(M fun) const = 0;

  virtual std::set<N> getExitPointsOf(M fun) const = 0;

  virtual std::set<N> getReturnSitesOfCallAt(N stmt) const = 0;

  virtual json getAsJson() const = 0;
};

} // namespace psr

#endif
