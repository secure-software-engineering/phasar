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

#include <map>
#include <set>
#include <string>
#include <vector>

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"

#include "nlohmann/json.hpp"

#include "phasar/PhasarLLVM/ControlFlow/CFG.h"

namespace psr {

enum class CallGraphAnalysisType {
#define ANALYSIS_SETUP_CALLGRAPH_TYPE(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/AnalysisSetups.def"
  Invalid
};

std::string toString(const CallGraphAnalysisType &CGA);

CallGraphAnalysisType toCallGraphAnalysisType(const std::string &S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const CallGraphAnalysisType &CGA);

template <typename N, typename F> class ICFG : public virtual CFG<N, F> {

protected:
  std::vector<F> GlobalInitializers;

  virtual void collectGlobalCtors() = 0;

  virtual void collectGlobalDtors() = 0;

  virtual void collectGlobalInitializers() = 0;

  virtual void collectRegisteredDtors() = 0;

public:
  ~ICFG() override = default;

  [[nodiscard]] virtual std::set<F> getAllFunctions() const = 0;

  [[nodiscard]] virtual F getFunction(const std::string &Fun) const = 0;

  [[nodiscard]] virtual bool isIndirectFunctionCall(N Stmt) const = 0;

  [[nodiscard]] virtual bool isVirtualFunctionCall(N Stmt) const = 0;

  [[nodiscard]] virtual std::set<N> allNonCallStartNodes() const = 0;

  [[nodiscard]] virtual std::set<F> getCalleesOfCallAt(N Stmt) const = 0;

  [[nodiscard]] virtual std::set<N> getCallersOf(F Fun) const = 0;

  [[nodiscard]] virtual std::set<N> getCallsFromWithin(F Fun) const = 0;

  [[nodiscard]] virtual std::set<N> getReturnSitesOfCallAt(N Stmt) const = 0;

  [[nodiscard]] const std::vector<F> &getGlobalInitializers() const {
    return GlobalInitializers;
  }

  using CFG<N, F>::print; // tell the compiler we wish to have both prints
  virtual void print(llvm::raw_ostream &OS = llvm::outs()) const = 0;

  using CFG<N, F>::getAsJson; // tell the compiler we wish to have both prints
  [[nodiscard]] virtual nlohmann::json getAsJson() const = 0;
};

} // namespace psr

#endif
