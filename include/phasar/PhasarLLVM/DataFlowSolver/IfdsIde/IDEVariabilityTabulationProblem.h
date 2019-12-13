#pragma once
#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <z3++.h>
namespace psr {
template <typename N, typename D, typename M, typename T, typename V,
          typename L, typename I>
class IDEVariabilityTabulationProblem
    : public IDETabulationProblem<N, D, M, V,
                                  VariationalICFG<N, M, z3::expr> &> {
  IDETabulationProblem<N, D, M, T, V, L, VariationalICFG<N, M, z3::expr> &>
      &problem;

public:
  IDEVariabilityTabulationProblem(
      IDETabulationProblem<N, D, M, T, V, L, I> &problem,
      LLVMBasedVariationalICFG &ICFg)
      : IDETabulationProblem(
            problem.getProjectIRDB(), problem.getTypeHierarchy(), &ICFg,
            problem.getPointsToInfo(), problem.getEntryPoints()),
        problem(problem) {}
  // Flow functions
  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr,
                                                         N succ) override {
    return problem.getNormalFlowFunction(curr, succ);
  }
  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                       M destMthd) override {
    return problem.getCallFlowFunction(callStmt, destMthd);
  }
  std::shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, M calleeMthd, N exitStmt, N retSite) override {
    return problem.getRetFlowFunction(callSite, calleeMthd, exitStmt, retSite);
  }
  std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite,
                           std::set<M> callees) override {
    return problem.getCallToRetFlowFunction(callSite, retSite, callees);
  }
  virtual std::shared_ptr<FlowFunction<D>>
  getSummaryFlowFunction(N curr, M destMthd) override {
    return problem.getSummaryFlowFunction(curr, destMthd);
  }

  // Edge functions
  virtual std::shared_ptr<EdgeFunction<V>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    auto userEdgeFn =
        problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
    z3::expr cond = this->ICF->getTrueCondition();

    this->ICF->isPPBranchTarget(curr, succ, cond);
    // if it is not a conditional branch, cond is true
    return std::make_shared<VariationalEdgeFunction<V>>(userEdgeFn, cond);
  }
  virtual std::shared_ptr<EdgeFunction<V>>
  getCallEdgeFunction(N callStmt, D srcNode, M destinationMethod,
                      D destNode) override {
    auto userEdgeFn = problem.getCallEdgeFunction(callStmt, srcNode,
                                                  destinationMethod, destNode);
    return std::make_shared<VariationalEdgeFunction<V>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }
  virtual std::shared_ptr<EdgeFunction<V>>
  getReturnEdgeFunction(N callSite, M calleeMethod, N exitStmt, D exitNode,
                        N reSite, D retNode) override {
    auto userEdgeFn = problem.getReturnEdgeFunction(
        callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    return std::make_shared<VariationalEdgeFunction<V>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }
  virtual std::shared_ptr<EdgeFunction<V>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<M> callees) override {
    auto userEdgeFn = problem.getCallToRetEdgeFunction(
        callSite, callNode, retSite, retSiteNode, callees);
    return std::make_shared<VariationalEdgeFunction<V>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }
  virtual std::shared_ptr<EdgeFunction<V>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    auto userEdgeFn =
        problem.getSummaryEdgeFunction(curr, currNode, succ, succNode);
    return std::make_shared<VariationalEdgeFunction<V>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }
};
} // namespace psr