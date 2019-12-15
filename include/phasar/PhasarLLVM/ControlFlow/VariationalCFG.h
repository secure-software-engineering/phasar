#pragma once
#include <phasar/PhasarLLVM/ControlFlow/CFG.h>
#include <tuple>
#include <vector>

namespace psr {
template <typename N, typename M, typename C>
class VariationalCFG : public virtual CFG<N, M> {

public:
  virtual ~VariationalCFG() = default;
  virtual bool isPPBranchTarget(N stmt, N succ) const = 0;
  /// \brief True, iff succ is a successor nod of stmt by an #ifdef branch
  ///
  /// \param condition The #ifdef condition if (stmt, succ) is a #ifdef branch,
  /// or else true
  virtual bool isPPBranchTarget(N stmt, N succ, C &condition) const = 0;
  virtual bool isNormalBranchTarget(N stmt, N succ) const {
    return this->isBranchTarget(stmt, succ) &&
           !this->isPPBranchTarget(stmt, succ);
  }
  virtual std::vector<std::tuple<N, C>> getSuccsOfWithCond(N stmt) = 0;
  virtual C getTrueCondition() const = 0;
  virtual std::vector<std::tuple<N, N, C>>
  getAllControlFlowEdgesWithCondition(M fun) {
    // TODO: make more (memory) efficient
    std::vector<std::tuple<N, N, C>> ret;
    auto normalCFGEdges = this->getAllControlFlowEdges(fun);
    ret.reserve(normalCFGEdges.size());

    for (auto &[curr, succ] : normalCFGEdges) {
      C cond = getTrueCondition();
      if (this->isPPBranchTarget(curr, succ, cond))
        ret.emplace_back(curr, succ, cond);
      else
        ret.emplace_back(curr, succ, getTrueCondition());
    }
    return ret;
  }
};
} // namespace psr