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

#include <map>

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
    : public IDETabulationProblem<N, D, F, T, V, VarL<L>,
                                  VariationalICFG<N, F, z3::expr>> {
public:
  using n_t = N;
  using d_t = D;
  using f_t = F;
  using t_t = T;
  using v_t = V;
  using user_l_t = L;
  // override l_t and i_t to capture the variability
  using l_t = VarL<L>;
  using i_t = VariationalICFG<N, F, z3::expr>;

private:
  IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem;
  LLVMBasedVariationalICFG &VarICF;

  const z3::expr TRUE_CONSTRAINT = VarICF.getTrueConstraint();
  const l_t BOTTOM = {{TRUE_CONSTRAINT, IDEProblem.bottomElement()}};
  const l_t TOP = {{TRUE_CONSTRAINT, IDEProblem.topElement()}};

public:
  IDEVariabilityTabulationProblem(
      IDETabulationProblem<N, D, F, T, V, L, I> &IDEProblem,
      LLVMBasedVariationalICFG &VarICF)
      : IDETabulationProblem<N, D, F, T, V, l_t,
                             VariationalICFG<N, F, z3::expr>>(
            IDEProblem.getProjectIRDB(), IDEProblem.getTypeHierarchy(), &VarICF,
            IDEProblem.getPointstoInfo(), IDEProblem.getEntryPoints()),
        IDEProblem(IDEProblem), VarICF(VarICF) {
    IDEVariabilityTabulationProblem::ZeroValue = createZeroValue();
  }

  // TODO also allow for solving IFDSTabulationProblems
  // IDEVariabilityTabulationProblem(IFDSTabulationProblem ...) {}

  ~IDEVariabilityTabulationProblem() override = default;

  // Flow functions
  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr,
                                                         N succ) override {
    // std::cout << "IDEVariabilityTabulationProblem::getNormalFlowFunction
    // applied on : "
    //           << IDEProblem.NtoString(curr)
    //           << '\n';
    // TODO
    // we need some kind of bool isPPrelatedInstruction(N stmt); that triggers
    // for all preprocessor related instructions
    // user problem needs to ignore all preprocessor related instructions
    // e.g. the following instructions must be ignored:
    //  - %0 = load i32, i32* @_Djkifd_CONFIG_A_defined, align 4
    //  - %tobool = icmp ne i32 %0, 0
    //  - br i1 %tobool, label %if.then, label %if.else
    // if (VarICF.isPPBranchTarget(curr, succ)) {
    //   return Identity<D>::getInstance();
    // }
    // otherwise just apply the user edge functions
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
      // std::cout << "\tD1: " << IDEProblem.DtoString(currNode) << '\n';
      // std::cout << "\tN : " << IDEProblem.NtoString(curr) << '\n';
      // std::cout << "\tD2: " << IDEProblem.DtoString(succNode) << '\n';
      // std::cout << "\tS : " << IDEProblem.NtoString(succ) << '\n';
      // return std::make_shared<>(EdgeIdentity<l_t>::getInstance(),
      // VarICF.getPPConstraintOrTrue(curr, succ));
    }
    // ordinary instruction, no preprocessor constraints
    std::cout << "Edge Function: " << *UserEF << '\n';
    return std::make_shared<VariationalEdgeFunction<user_l_t>>(UserEF,
                                                               TRUE_CONSTRAINT);
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
    return nullptr;
  }

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override {
    auto Top = std::make_shared<AllTop<l_t>>(TOP);
    return Top;
  }

  l_t topElement() override { return TOP; }

  l_t bottomElement() override { return BOTTOM; }

  l_t join(l_t Lhs, l_t Rhs) override {
    // std::cout << "IDEVariabilityTabulationProblem::join\n";
    // std::cout << "lhs: ";
    // printEdgeFact(std::cout, Lhs);
    // std::cout << "rhs: ";
    // printEdgeFact(std::cout, Rhs);
    // std::cout << " --> ";
    for (auto &[LConstraint, LValue] : Lhs) {
      // case Rhs already contains the constraint
      if (Rhs.count(LConstraint)) {
        Rhs[LConstraint] = IDEProblem.join(LValue, Rhs[LConstraint]);
      } else {
        // otherwise add the new <constraint, value> pair to Rhs
        Rhs[LConstraint] = LValue;
      }
    }
    // printEdgeFact(std::cout, Rhs);
    // std::cout << '\n';
    return Rhs;
  }

  D createZeroValue() const override {
    // ZeroValue should have been generated by the ctor of the original problem
    // IDEProblem.createZeroValue()
    return IDEProblem.getZeroValue();
  }

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
    for (auto &[Constraint, Value] : l) {
      os << '<' << Constraint.to_string() << " , ";
      IDEProblem.printEdgeFact(os, Value);
      os << "> ; ";
    }
  }
};

template <typename Problem>
IDEVariabilityTabulationProblem(Problem &)
    ->IDEVariabilityTabulationProblem<
        typename Problem::n_t, typename Problem::d_t, typename Problem::f_t,
        typename Problem::t_t, typename Problem::v_t, typename Problem::l_t,
        typename Problem::i_t>;

template <typename Problem>
using IDEVariabilityTabulationProblem_P = IDEVariabilityTabulationProblem<
    typename Problem::n_t, typename Problem::d_t, typename Problem::f_t,
    typename Problem::t_t, typename Problem::v_t, typename Problem::l_t,
    typename Problem::i_t>;

} // namespace psr

#endif
