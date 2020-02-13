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
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VariationalEdgeFunction.h>

namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class IDEVariabilityTabulationProblem
    : public IDETabulationProblem<N, D, F, T, V, std::pair<L, z3::expr>,
                                  VariationalICFG<N, F, z3::expr>> {
public:
  using l_t = std::pair<L, z3::expr>;

private:
  IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem;
  LLVMBasedVariationalICFG &VarICF;

public:
  IDEVariabilityTabulationProblem(
      IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem,
      LLVMBasedVariationalICFG &VarICF)
      : IDETabulationProblem<N, D, F, T, V, l_t,
                             VariationalICFG<N, F, z3::expr>>(
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
    //     << "IDEVariabilityTabulationProblem::getNormalFlowFunction applied
    //     on: "
    //     << IDEProblem.NtoString(curr) << '\n';
    return IDEProblem.getNormalFlowFunction(curr, succ);
  }

  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                       F destMthd) override {
    // std::cout << "IDEVariabilityTabulationProblem::getCallFlowFunction\n";
    return Identity<D>::getInstance();
  }

  std::shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, F calleeMthd, N exitStmt, N retSite) override {
    // std::cout << "IDEVariabilityTabulationProblem::getRetFlowFunction\n";
    return Identity<D>::getInstance();
  }

  std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite,
                           std::set<F> callees) override {
    // std::cout <<
    // "IDEVariabilityTabulationProblem::getCallToRetFlowFunction\n";
    return Identity<D>::getInstance();
  }

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N curr,
                                                          F destMthd) override {
    return nullptr;
  }

  // Edge functions
  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    auto UserEF =
        IDEProblem.getNormalEdgeFunction(curr, currNode, succ, succNode);
    // curr is a special preprocessor #ifdef instruction, we need to add a
    // preprocessor constraint
    if (VarICF.isPPBranchTarget(curr, succ)) {
      std::cout << "PP-Edge constaint: "
                << VarICF.getPPConstraintOrTrue(curr, succ).to_string() << '\n';
      std::cout << "\tD1: " << IDEProblem.DtoString(currNode) << '\n';
      std::cout << "\tN : " << IDEProblem.NtoString(curr) << '\n';
      std::cout << "\tD2: " << IDEProblem.DtoString(succNode) << '\n';
      std::cout << "\tS : " << IDEProblem.NtoString(succ) << '\n';
      // return std::make_shared<>(EdgeIdentity<l_t>::getInstance(),
      // VarICF.getPPConstraintOrTrue(curr, succ));
    }
    // ordinary instruction, no preprocessor constraints
    return std::make_shared<VariationalEdgeFunction<l_t>>(
        UserEF, EdgeIdentity<z3::expr>::getInstance());
  }

  std::shared_ptr<EdgeFunction<l_t>> getCallEdgeFunction(N callStmt, D srcNode,
                                                         F destinationMethod,
                                                         D destNode) override {
    // auto userEdgeFn = IDEProblem.getCallEdgeFunction(
    //     callStmt, srcNode, destinationMethod, destNode);
    // return std::make_shared<VariationalEdgeFunction<l_t>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
  }

  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(N callSite, F calleeMethod, N exitStmt, D exitNode,
                        N reSite, D retNode) override {
    // auto userEdgeFn = IDEProblem.getReturnEdgeFunction(
    //     callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    // return std::make_shared<VariationalEdgeFunction<l_t>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
  }

  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<F> callees) override {
    // auto userEdgeFn = IDEProblem.getCallToRetEdgeFunction(
    //     callSite, callNode, retSite, retSiteNode, callees);
    // return std::make_shared<VariationalEdgeFunction<l_t>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
  }

  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) override {
    // auto userEdgeFn =
    //     IDEProblem.getSummaryEdgeFunction(curr, currNode, succ, succNode);
    // return std::make_shared<VariationalEdgeFunction<l_t>>(
    //     userEdgeFn, this->ICF->getTrueCondition());
    return EdgeIdentity<l_t>::getInstance();
  }

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override {
    static auto Top = std::make_shared<AllTop<l_t>>(
        std::make_pair(IDEProblem.topElement(), VarICF.getTrueConstraint()));
    return Top;
  }

  l_t topElement() override {
    return std::make_pair(IDEProblem.topElement(), VarICF.getTrueConstraint());
  }

  l_t bottomElement() override {
    return std::make_pair(IDEProblem.bottomElement(),
                          VarICF.getTrueConstraint());
  }

  l_t join(l_t lhs, l_t rhs) override {
    return std::make_pair(IDEProblem.join(lhs.first, rhs.first),
                          VarICF.getTrueConstraint());
  }

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

  void printEdgeFact(std::ostream &os, l_t l) const override {
    os << '<';
    IDEProblem.printEdgeFact(os, l.first);
    os << " , " << l.second.to_string() << '>';
  }
};

} // namespace psr

#endif
