#pragma once
#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VariationalEdgeFunction.h>
#include <z3++.h>
namespace psr {
template <typename N, typename D, typename M, typename T, typename V,
          typename L, typename I>
class IDEVariabilityTabulationProblem
    : public IDETabulationProblem<N, D, M, T, V, L,
                                  VariationalICFG<N, M, z3::expr>> {
  IDETabulationProblem<N, D, M, T, V, L, I> &problem;

public:
  IDEVariabilityTabulationProblem(
      IDETabulationProblem<N, D, M, T, V, L, I> &problem,
      LLVMBasedVariationalICFG &ICFg)
      : IDETabulationProblem<N, D, M, T, V, L, VariationalICFG<N, M, z3::expr>>(
            problem.getProjectIRDB(), problem.getTypeHierarchy(), &ICFg,
            problem.getPointstoInfo(), problem.getEntryPoints()),
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
  virtual std::shared_ptr<EdgeFunction<L>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    auto userEdgeFn =
        problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
    z3::expr cond = this->ICF->getTrueCondition();

    this->ICF->isPPBranchTarget(curr, succ, cond);
    // if it is not a conditional branch, cond is true
    return std::make_shared<VariationalEdgeFunction<L>>(userEdgeFn, cond);
  }
  virtual std::shared_ptr<EdgeFunction<L>>
  getCallEdgeFunction(N callStmt, D srcNode, M destinationMethod,
                      D destNode) override {
    auto userEdgeFn = problem.getCallEdgeFunction(callStmt, srcNode,
                                                  destinationMethod, destNode);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }
  virtual std::shared_ptr<EdgeFunction<L>>
  getReturnEdgeFunction(N callSite, M calleeMethod, N exitStmt, D exitNode,
                        N reSite, D retNode) override {
    auto userEdgeFn = problem.getReturnEdgeFunction(
        callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }
  virtual std::shared_ptr<EdgeFunction<L>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<M> callees) override {
    auto userEdgeFn = problem.getCallToRetEdgeFunction(
        callSite, callNode, retSite, retSiteNode, callees);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }
  virtual std::shared_ptr<EdgeFunction<L>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    auto userEdgeFn =
        problem.getSummaryEdgeFunction(curr, currNode, succ, succNode);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }

  void printNode(std::ostream &os, N n) const override {
    problem.printNode(os, n);
  }
  void printDataFlowFact(std::ostream &os, D d) const override {
    problem.printDataFlowFact(os, d);
  }
  void printMethod(std::ostream &os, M m) const override {
    problem.printMethod(os, m);
  }
  D createZeroValue() const override { return problem.createZeroValue(); }
  bool isZeroValue(D d) const override { return problem.isZeroValue(d); }
  std::map<N, std::set<D>> initialSeeds() override {
    return problem.initialSeeds();
  }
  L topElement() override { return problem.topElement(); }
  L bottomElement() override { return problem.bottomElement(); }
  L join(L lhs, L rhs) override { return problem.join(lhs, rhs); }
  void printEdgeFact(std::ostream &os, L l) const override {
    problem.printEdgeFact(os, l);
  }
  std::shared_ptr<EdgeFunction<L>> allTopFunction() override {
    static std::shared_ptr<EdgeFunction<L>> allTop =
        std::make_shared<VariationalEdgeFunction<L>>(
            problem.allTopFunction(), this->ICF->getTrueCondition());
    return allTop;
  }
};
} // namespace psr