/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDEVARIABILITYTABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDEVARIABILITYTABULATIONPROBLEM_H_

#include <z3++.h>

#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VariationalEdgeFunction.h>

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class IDEVariabilityTabulationProblem
    : public IDETabulationProblem<N, D, F, T, V, L,
                                  VariationalICFG<N, F, z3::expr>> {
  IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem;

public:
  IDEVariabilityTabulationProblem(
      IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem,
      LLVMBasedVariationalICFG &VarICF)
      : IDETabulationProblem<N, D, F, T, V, L, VariationalICFG<N, F, z3::expr>>(
            IDEProblem.getProjectIRDB(), IDEProblem.getTypeHierarchy(), &VarICF,
            IDEProblem.getPointstoInfo(), IDEProblem.getEntryPoints()),
        IDEProblem(IDEProblem) {}

  // TODO also allow for solving IFDSTabulationProblems
  // IDEVariabilityTabulationProblem(IFDSTabulationProblem ...) {}

  ~IDEVariabilityTabulationProblem() override = default;

  // Flow functions
  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr,
                                                         N succ) override {
    return IDEProblem.getNormalFlowFunction(curr, succ);
  }

  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                       F destMthd) override {
    return IDEProblem.getCallFlowFunction(callStmt, destMthd);
  }

  std::shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, F calleeMthd, N exitStmt, N retSite) override {
    return IDEProblem.getRetFlowFunction(callSite, calleeMthd, exitStmt,
                                         retSite);
  }

  std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite,
                           std::set<F> callees) override {
    return IDEProblem.getCallToRetFlowFunction(callSite, retSite, callees);
  }

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N curr,
                                                          F destMthd) override {
    return IDEProblem.getSummaryFlowFunction(curr, destMthd);
  }

  // Edge functions
  std::shared_ptr<EdgeFunction<L>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    auto userEdgeFn =
        IDEProblem.getNormalEdgeFunction(curr, currNode, succ, succNode);
    z3::expr cond = this->ICF->getTrueCondition();

    this->ICF->isPPBranchTarget(curr, succ, cond);
    // if it is not a conditional branch, cond is true
    return std::make_shared<VariationalEdgeFunction<L>>(userEdgeFn, cond);
  }

  std::shared_ptr<EdgeFunction<L>> getCallEdgeFunction(N callStmt, D srcNode,
                                                       F destinationMethod,
                                                       D destNode) override {
    auto userEdgeFn = IDEProblem.getCallEdgeFunction(
        callStmt, srcNode, destinationMethod, destNode);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }

  std::shared_ptr<EdgeFunction<L>>
  getReturnEdgeFunction(N callSite, F calleeMethod, N exitStmt, D exitNode,
                        N reSite, D retNode) override {
    auto userEdgeFn = IDEProblem.getReturnEdgeFunction(
        callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }

  std::shared_ptr<EdgeFunction<L>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<F> callees) override {
    auto userEdgeFn = IDEProblem.getCallToRetEdgeFunction(
        callSite, callNode, retSite, retSiteNode, callees);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }

  std::shared_ptr<EdgeFunction<L>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    auto userEdgeFn =
        IDEProblem.getSummaryEdgeFunction(curr, currNode, succ, succNode);
    return std::make_shared<VariationalEdgeFunction<L>>(
        userEdgeFn, this->ICF->getTrueCondition());
  }

  std::shared_ptr<EdgeFunction<L>> allTopFunction() override {
    static std::shared_ptr<EdgeFunction<L>> allTop =
        std::make_shared<VariationalEdgeFunction<L>>(
            IDEProblem.allTopFunction(), this->ICF->getTrueCondition());
    return allTop;
  }

  L topElement() override { return IDEProblem.topElement(); }

  L bottomElement() override { return IDEProblem.bottomElement(); }

  L join(L lhs, L rhs) override { return IDEProblem.join(lhs, rhs); }

  void printNode(std::ostream &os, N n) const override {
    IDEProblem.printNode(os, n);
  }

  void printDataFlowFact(std::ostream &os, D d) const override {
    IDEProblem.printDataFlowFact(os, d);
  }

  void printFunction(std::ostream &os, F m) const override {
    IDEProblem.printFunction(os, m);
  }

  D createZeroValue() const override { return IDEProblem.createZeroValue(); }

  bool isZeroValue(D d) const override { return IDEProblem.isZeroValue(d); }

  std::map<N, std::set<D>> initialSeeds() override {
    return IDEProblem.initialSeeds();
  }

  void printEdgeFact(std::ostream &os, L l) const override {
    IDEProblem.printEdgeFact(os, l);
  }
};

} // namespace psr

#endif
