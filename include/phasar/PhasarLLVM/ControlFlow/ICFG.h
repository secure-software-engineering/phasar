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
#include <vector>

#include "llvm/ADT/DenseMap.h"

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

std::ostream &operator<<(std::ostream &os, const CallGraphAnalysisType &CGA);

template <typename N, typename F> class ICFG : public virtual CFG<N, F> {
public:
  using GlobalCtorTy = std::multimap<size_t, F>;
  using GlobalDtorTy = std::multimap<size_t, F, std::greater<size_t>>;

protected:
  std::vector<F> GlobalInitializers, RegisteredDtors;

  GlobalCtorTy GlobalCtors;
  GlobalDtorTy GlobalDtors;

  llvm::SmallDenseMap<F, typename GlobalCtorTy::const_iterator, 2> GlobalCtorFn;
  llvm::SmallDenseMap<F, typename GlobalDtorTy::const_iterator, 2> GlobalDtorFn;
  llvm::SmallDenseMap<F, size_t, 8> RegisteredDtorIndex;

  virtual void collectGlobalCtors() = 0;

  virtual void collectGlobalDtors() = 0;

  virtual void collectGlobalInitializers() = 0;

  virtual void collectRegisteredDtors() = 0;

public:
  ~ICFG() override = default;

  virtual std::set<F> getAllFunctions() const = 0;

  virtual F getFunction(const std::string &Fun) const = 0;

  virtual bool isIndirectFunctionCall(N Stmt) const = 0;

  virtual bool isVirtualFunctionCall(N Stmt) const = 0;

  virtual std::set<N> allNonCallStartNodes() const = 0;

  virtual std::set<F> getCalleesOfCallAt(N Stmt) const = 0;

  virtual std::set<N> getCallersOf(F Fun) const = 0;

  virtual std::set<N> getCallsFromWithin(F Fun) const = 0;

  virtual std::set<N> getReturnSitesOfCallAt(N Stmt) const = 0;

  const GlobalCtorTy &getGlobalCtors() const { return GlobalCtors; }

  template <typename Fn> void forEachGlobalCtor(Fn &&fn) const {
    for (auto [Prio, Fun] : GlobalCtors) {
      fn(Fun);
    }
  }

  void appendGlobalCtors(std::vector<F> &CtorsList) const {
    CtorsList.reserve(CtorsList.size() + GlobalCtors.size());
    forEachGlobalCtor([&](auto *Fun) { CtorsList.push_back(Fun); });
  }

  const GlobalDtorTy &getGlobalDtors() const { return GlobalDtors; }

  template <typename Fn> void forEachGlobalDtor(Fn &&fn) const {
    for (auto [Prio, Fun] : GlobalDtors) {
      fn(Fun);
    }
  }

  void appendGlobalDtors(std::vector<F> &DtorsList) const {
    DtorsList.reserve(DtorsList.size() + GlobalDtors.size());
    forEachGlobalDtor([&](auto *Fun) { DtorsList.push_back(Fun); });
  }

  const std::vector<F> &getGlobalInitializers() const {
    return GlobalInitializers;
  }

  const std::vector<F> &getRegisteredDtors() const { return RegisteredDtors; }

  using CFG<N, F>::print; // tell the compiler we wish to have both prints
  virtual void print(std::ostream &OS = std::cout) const = 0;

  using CFG<N, F>::getAsJson; // tell the compiler we wish to have both prints
  virtual nlohmann::json getAsJson() const = 0;
};

} // namespace psr

#endif
