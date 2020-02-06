/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CFG.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_CFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_CFG_H_

#include <string>
#include <utility> // std::pair
#include <vector>

#include <nlohmann/json.hpp>

namespace psr {

template <typename N, typename F> class CFG {
public:
  virtual ~CFG() = default;

  virtual F getFunctionOf(N stmt) const = 0;

  virtual std::vector<N> getPredsOf(N stmt) const = 0;

  virtual std::vector<N> getSuccsOf(N stmt) const = 0;

  virtual std::vector<std::pair<N, N>> getAllControlFlowEdges(F fun) const = 0;

  virtual std::vector<N> getAllInstructionsOf(F fun) const = 0;

  virtual bool isExitStmt(N stmt) const = 0;

  virtual bool isStartPoint(N stmt) const = 0;

  virtual bool isFieldLoad(N stmt) const = 0;

  virtual bool isFieldStore(N stmt) const = 0;

  virtual bool isFallThroughSuccessor(N stmt, N succ) const = 0;

  virtual bool isBranchTarget(N stmt, N succ) const = 0;

  virtual std::string getStatementId(N stmt) const = 0;

  virtual std::string getFunctionName(F fun) const = 0;

  virtual void print(F fun, std::ostream &OS) const = 0;

  virtual nlohmann::json getAsJson(F fun) const = 0;
};

} // namespace psr

#endif
