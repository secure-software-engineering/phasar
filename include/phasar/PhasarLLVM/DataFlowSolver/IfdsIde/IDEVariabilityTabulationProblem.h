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

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VariationalEdgeFunction.h>

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class IDEVariabilityTabulationProblem
    : public IDETabulationProblem<N, D, F, T, V, L,
                                  VariationalICFG<N, F, z3::expr>> {
private:
  IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem;
  LLVMBasedVariationalICFG &VarICF;

public:
  using d_t = D;
  using l_t = L;

  IDEVariabilityTabulationProblem(
      IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem,
      LLVMBasedVariationalICFG &VarICF)
      : IDETabulationProblem<N, D, F, T, V, L, VariationalICFG<N, F, z3::expr>>(
            IDEProblem.getProjectIRDB(), IDEProblem.getTypeHierarchy(), &VarICF,
            IDEProblem.getPointstoInfo(), IDEProblem.getEntryPoints()),
        IDEProblem(IDEProblem), VarICF(VarICF) {}

  // TODO also allow for solving IFDSTabulationProblems
  // IDEVariabilityTabulationProblem(IFDSTabulationProblem ...) {}

  ~IDEVariabilityTabulationProblem() override = default;

  // Flow functions
  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr,
                                                         N succ) override {
    // std::cout
    //     << "IDEVariabilityTabulationProblem::getNormalFlowFunction applied on: "
    //     << IDEProblem.NtoString(curr) << '\n';
    return IDEProblem.getNormalFlowFunction(curr, succ);
  }

  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                       F destMthd) override {
    // std::cout << "IDEVariabilityTabulationProblem::getCallFlowFunction\n";
    return Identity<d_t>::getInstance();
  }

  std::shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, F calleeMthd, N exitStmt, N retSite) override {
    // std::cout << "IDEVariabilityTabulationProblem::getRetFlowFunction\n";
    return Identity<d_t>::getInstance();
  }

  std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite,
                           std::set<F> callees) override {
    // std::cout << "IDEVariabilityTabulationProblem::getCallToRetFlowFunction\n";
    return Identity<d_t>::getInstance();
  }

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N curr,
                                                          F destMthd) override {
    return nullptr;
  }

  // Edge functions
  std::shared_ptr<EdgeFunction<L>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    // std::cout
    //     << "IDEVariabilityTabulationProblem::getNormalEdgeFunction applied
    //     on: "
    //     << IDEProblem.NtoString(curr) << '\n';
    auto UserEF =
        IDEProblem.getNormalEdgeFunction(curr, currNode, succ, succNode);

    if (VarICF.isPPBranchTarget(curr, succ)) {
      std::cout << "Found PP branch target spawning from: " << IDEProblem.NtoString(curr) << '\n';
      std::cout << "\tConstraint: " << VarICF.getPPConstraintOrTrue(curr, succ).to_string() << '\n';
    }
    // this->ICF->isPPBranchTarget(curr, succ, Constraint);
    // if it is not a conditional branch, Constraint is true
    auto Constraint = VarICF.getTrueCondition();
    return std::make_shared<VariationalEdgeFunction<L>>(UserEF, Constraint);
  }

  std::shared_ptr<EdgeFunction<L>> getCallEdgeFunction(N callStmt, D srcNode,
                                                       F destinationMethod,
                                                       D destNode) override {
    // auto userEdgeFn = IDEProblem.getCallEdgeFunction(
    //     callStmt, srcNode, destinationMethod, destNode);
    // return std::make_shared<VariationalEdgeFunction<L>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
  }

  std::shared_ptr<EdgeFunction<L>>
  getReturnEdgeFunction(N callSite, F calleeMethod, N exitStmt, D exitNode,
                        N reSite, D retNode) override {
    // auto userEdgeFn = IDEProblem.getReturnEdgeFunction(
    //     callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    // return std::make_shared<VariationalEdgeFunction<L>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
  }

  std::shared_ptr<EdgeFunction<L>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<F> callees) override {
    // auto userEdgeFn = IDEProblem.getCallToRetEdgeFunction(
    //     callSite, callNode, retSite, retSiteNode, callees);
    // return std::make_shared<VariationalEdgeFunction<L>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
  }

  std::shared_ptr<EdgeFunction<L>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    // auto userEdgeFn =
    //     IDEProblem.getSummaryEdgeFunction(curr, currNode, succ, succNode);
    // return std::make_shared<VariationalEdgeFunction<L>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
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

  D createZeroValue() const override { return IDEProblem.createZeroValue(); }

  bool isZeroValue(D d) const override { return IDEProblem.isZeroValue(d); }

  std::map<N, std::set<D>> initialSeeds() override {
    return IDEProblem.initialSeeds();
  }

  void printNode(std::ostream &os, N n) const override {
    IDEProblem.printNode(os, n);
  }

  void printDataFlowFact(std::ostream &os, D d) const override {
    IDEProblem.printDataFlowFact(os, d);
  }

  void printFunction(std::ostream &os, F m) const override {
    IDEProblem.printFunction(os, m);
  }

  void printEdgeFact(std::ostream &os, L l) const override {
    IDEProblem.printEdgeFact(os, l);
  }
};

} // namespace psr

#endif
