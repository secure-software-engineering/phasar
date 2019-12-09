#pragma once
#include "CFG.h"
#include <tuple>
#include <vector>

namespace psr {
template <typename N, typename M, typename C>
class VariationalCFG : public virtual CFG<N, M> {

public:
  virtual ~VariationalCFG() = default;
  virtual bool isPPBranchTarget(N stmt, N succ) = 0;
  /// \brief True, iff succ is a successor nod of stmt by an #ifdef branch
  ///
  /// \param condition The #ifdef condition if (stmt, succ) is a #ifdef branch,
  /// or else true
  virtual bool isPPBranchTarget(N stmt, N succ, C &condition) = 0;
  virtual bool isNormalBranchTarget(N stmt, N succ) {
    return isBranchTarget(stmt, succ) && !isPPBranchTarget(stmt, succ);
  }
  virtual std::vectorstd::tuple<N, C>> getSuccsOfWithCond(N stmt) = 0;
  virtual std::vector<std::tuple<N, N, C>>
  getAllControlFlowEdgesWithCondition(M fun) {
    // TODO: make more (memory) efficient
    std::vector<std::tuple<N, N, C>> ret;
    auto normalCFGEdges = getAllControlFlowEdges();
    ret.reserve(normalCFGEdges.size());

    for (auto &[curr, succ] : normalCFGEdges) {
      C cond;
      if (isPPBranchTarget(curr, succ, cond))
        ret.emplace_back(curr, succ, cond);
      else
        ret.emplace_back(curr, succ, getTrueCondition());
    }
    return ret;
  }
  virtual C getTrueCondition() = 0;
};
} // namespace psr