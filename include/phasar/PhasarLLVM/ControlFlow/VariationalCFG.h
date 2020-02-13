/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_VARIATIONALCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_VARIATIONALCFG_H_

#include <tuple>
#include <utility>
#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/CFG.h>

namespace psr {

template <typename N, typename F, typename C>
class VariationalCFG : public virtual CFG<N, F> {
public:
  virtual ~VariationalCFG() = default;

  /// \brief True, iff Succ is a successor node of Stmt by an #ifdef branch, or
  /// else true
  virtual bool isPPBranchTarget(N Stmt, N Succ) const = 0;

  bool isNormalBranchTarget(N Stmt, N Succ) const {
    return this->isBranchTarget(Stmt, Succ) &&
           !this->isPPBranchTarget(Stmt, Succ);
  }

  /// \brief Returns the #ifdef PP constraint
  virtual C getPPConstraintOrTrue(N Stmt, N Succ) const = 0;

  virtual std::vector<std::pair<N, C>>
  getSuccsOfWithPPConstraints(N Stmt) const = 0;

  virtual C getTrueConstraint() const = 0;

  std::vector<std::tuple<N, N, C>>
  getAllControlFlowEdgesWithConstraints(F Fun) const {
    std::vector<std::tuple<N, N, C>> Res;
    auto NormalCFGEdges = this->getAllControlFlowEdges(Fun);
    Res.reserve(NormalCFGEdges.size());
    for (auto &[Curr, Succ] : NormalCFGEdges) {
      Res.emplace_back(Curr, Succ, this->getPPConstraintOrTrue(Curr, Succ));
    }
    return Res;
  }
};

} // namespace psr

#endif
