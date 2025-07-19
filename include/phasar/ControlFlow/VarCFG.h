/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_VARCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_VARCFG_H_

#include "phasar/ControlFlow/CFGBase.h"
#include "phasar/Utils/ByRef.h"

#include <tuple>
#include <utility>
#include <vector>

namespace psr {

template <typename CFGTy, typename C> class VarCFGImpl {
  // To Implement per CFG-C combination:

  // bool isPPBranchTargetImpl(n_t Stmt, n_t Succ) const noexcept;
  // C getPPConstraintOrTrueImpl(n_t Stmt, n_t Succ) const;
  // vector<pair<N, C>> getSuccsOfWithPPConstraintsImpl(n_t Stmt) const;
  // C getTrueConstraintImpl() const;

  // const CFG &getCFG() const;
  // VarCFGImpl(const CFG&, const stringstringmap_t *StaticBackwardRenaming);
};

template <typename CFGTy, typename C>
class VarCFG : private VarCFGImpl<CFGTy, C> {
  friend class VariabilityCFGTest;

public:
  using n_t = typename CFGTy::n_t;
  using f_t = typename CFGTy::f_t;

  using VarCFGImpl<CFGTy, C>::VarCFGImpl;

  /// \brief True, iff Succ is a successor node of Stmt by an #ifdef branch, or
  /// else true
  [[nodiscard]] bool isPPBranchTarget(n_t Stmt, n_t Succ) const noexcept {
    return this->isPPBranchTargetImpl(Stmt, Succ);
  }

  [[nodiscard]] bool isNormalBranchTarget(n_t Stmt, n_t Succ) const {
    return this->getCFG().isBranchTarget(Stmt, Succ) &&
           !isPPBranchTarget(Stmt, Succ);
  }

  [[nodiscard]] C getPPConstraintOrTrue(n_t Stmt, n_t Succ) const {
    return this->getPPConstraintOrTrueImpl(Stmt, Succ);
  }

  // std::vector<std::pair<N, C>>
  [[nodiscard]] decltype(auto) getSuccsOfWithPPConstraints(n_t Stmt) const {
    return this->getSuccsOfWithPPConstraintsImpl(Stmt);
  }

  [[nodiscard]] C getTrueConstraint() const {
    return this->getTrueConstraintImpl();
  }

  [[nodiscard]] std::vector<std::tuple<n_t, n_t, C>>
  getAllControlFlowEdgesWithConstraints(ByConstRef<f_t> Fun) const {
    std::vector<std::tuple<n_t, n_t, C>> Res;
    auto &&NormalCFGEdges = this->getCFG().getAllControlFlowEdges(Fun);
    Res.reserve(NormalCFGEdges.size());
    for (auto &[Curr, Succ] : NormalCFGEdges) {
      Res.emplace_back(Curr, Succ, getPPConstraintOrTrue(Curr, Succ));
    }
    return Res;
  }
};

} // namespace psr

#endif
