/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDEVARTABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IDEVARTABULATIONPROBLEM_H_

#include <map>
#include <memory>
#include <set>

#include <z3++.h>

#include "phasar/PhasarLLVM/ControlFlow/VarICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarEdgeFunctions.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

template <typename AnalysisDomainTy>
struct IDEVarProblemAnalysisDomainTransformer : public AnalysisDomainTy {
  // save user-problem l_t as underlying user_l_t
  using user_l_t = typename AnalysisDomainTy::l_t;
  // transform l_t to be a VarL<l_t>
  using l_t = VarL<typename AnalysisDomainTy::l_t>;
  // use a variational aware inter-procedural control-flow graph
  using i_t = VarICFG<typename AnalysisDomainTy::n_t,
                      typename AnalysisDomainTy::f_t, z3::expr>;
};

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDEVarTabulationProblem
    : public IDETabulationProblem<
          IDEVarProblemAnalysisDomainTransformer<AnalysisDomainTy>, Container> {
public:
  using VarAnalysisDomainTy =
      IDEVarProblemAnalysisDomainTransformer<AnalysisDomainTy>;
  using d_t = typename VarAnalysisDomainTy::d_t;
  using n_t = typename VarAnalysisDomainTy::n_t;
  using f_t = typename VarAnalysisDomainTy::f_t;
  using t_t = typename VarAnalysisDomainTy::t_t;
  using v_t = typename VarAnalysisDomainTy::v_t;
  using l_t = typename VarAnalysisDomainTy::l_t;
  using user_l_t = typename VarAnalysisDomainTy::user_l_t;
  using i_t = typename VarAnalysisDomainTy::i_t;

  using typename FlowFunctions<VarAnalysisDomainTy>::FlowFunctionPtrType;
  using typename EdgeFunctions<VarAnalysisDomainTy>::EdgeFunctionPtrType;

private:
  IDETabulationProblem<AnalysisDomainTy, Container> &IDEProblem;
  LLVMBasedVarICFG &VarICF;

  const z3::expr TRUE_CONSTRAINT = VarICF.getTrueConstraint();
  const l_t BOTTOM = {{TRUE_CONSTRAINT, IDEProblem.bottomElement()}};
  const l_t TOP = {{TRUE_CONSTRAINT, IDEProblem.topElement()}};

public:
  IDEVarTabulationProblem(
      IDETabulationProblem<AnalysisDomainTy, Container> &IDEProblem,
      LLVMBasedVarICFG &VarICF)
      : IDETabulationProblem<VarAnalysisDomainTy, Container>(
            IDEProblem.getProjectIRDB(), IDEProblem.getTypeHierarchy(), &VarICF,
            IDEProblem.getPointstoInfo(), IDEProblem.getEntryPoints()),
        IDEProblem(IDEProblem), VarICF(VarICF) {
    IDEVarTabulationProblem::ZeroValue = createZeroValue();
  }

  // TODO also allow for solving IFDSTabulationProblems
  // IDEVarTabulationProblem(IFDSTabulationProblem ...) {}

  ~IDEVarTabulationProblem() override = default;

  // Flow functions
  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override {
    // std::cout << "IDEVarTabulationProblem::getNormalFlowFunction applied to:
    // "
    //           << IDEProblem.NtoString(curr) << '\n';
    // // TODO
    // // we need some kind of bool isPPrelatedInstruction(n_t stmt); that
    // triggers
    // // for all preprocessor related instructions
    // // user problem needs to ignore all preprocessor related instructions
    // // e.g. the following instructions must be ignored:
    // //  - %0 = load i32, i32* @_Djkifd_CONFIG_A_defined, align 4
    // //  - %tobool = icmp ne i32 %0, 0
    // //  - br i1 %tobool, label %if.then, label %if.else
    if (VarICF.isPPBranchTarget(curr, succ)) {
      std::cout << "Found PP branch: " << llvmIRToString(curr) << '\n';
      //   //   return Identity<d_t>::getInstance();
    }
    // otherwise just apply the user edge functions
    return IDEProblem.getNormalFlowFunction(curr, succ);
  }

  FlowFunctionPtrType getCallFlowFunction(n_t callStmt, f_t destMthd) override {
    // std::cout << "IDEVarTabulationProblem::getCallFlowFunction\n";
    return IDEProblem.getCallFlowFunction(callStmt, destMthd);
  }

  FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeMthd,
                                         n_t exitStmt, n_t retSite) override {
    // std::cout << "IDEVarTabulationProblem::getRetFlowFunction\n";
    return IDEProblem.getRetFlowFunction(callSite, calleeMthd, exitStmt,
                                         retSite);
  }

  FlowFunctionPtrType getCallToRetFlowFunction(n_t callSite, n_t retSite,
                                               std::set<f_t> callees) override {
    // std::cout <<
    // "IDEVarTabulationProblem::getCallToRetFlowFunction\n";
    return IDEProblem.getCallToRetFlowFunction(callSite, retSite, callees);
  }

  FlowFunctionPtrType getSummaryFlowFunction(n_t curr, f_t destMthd) override {
    return nullptr;
  }

  // Edge functions
  EdgeFunctionPtrType getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                                            d_t succNode) override {
    auto UserEF =
        IDEProblem.getNormalEdgeFunction(curr, currNode, succ, succNode);
    // if curr is a special preprocessor #ifdef instruction, we need to add a
    // preprocessor constraint
    if (VarICF.isPPBranchTarget(curr, succ)) {
      std::cout << "PP-Edge constaint: "
                << VarICF.getPPConstraintOrTrue(curr, succ).to_string() << '\n';
      // std::cout << "\tD1: " << IDEProblem.DtoString(currNode) << '\n';
      // std::cout << "\tN : " << IDEProblem.NtoString(curr) << '\n';
      // std::cout << "\tD2: " << IDEProblem.DtoString(succNode) << '\n';
      // std::cout << "\tS : " << IDEProblem.NtoString(succ) << '\n';
      // return std::make_shared<>(EdgeIdentity<l_t>::getInstance(),
      return std::make_shared<VarEdgeFunction<user_l_t>>(
          UserEF, VarICF.getPPConstraintOrTrue(curr, succ));
    }
    // ordinary instruction, no preprocessor constraints
    std::cout << "Edge Function: " << *UserEF << '\n';
    return std::make_shared<VarEdgeFunction<user_l_t>>(UserEF, TRUE_CONSTRAINT);
  }

  EdgeFunctionPtrType getCallEdgeFunction(n_t callStmt, d_t srcNode,
                                          f_t destinationMethod,
                                          d_t destNode) override {
    auto UserEF = IDEProblem.getCallEdgeFunction(callStmt, srcNode,
                                                 destinationMethod, destNode);
    return std::make_shared<VarEdgeFunction<user_l_t>>(UserEF, TRUE_CONSTRAINT);
  }

  EdgeFunctionPtrType getReturnEdgeFunction(n_t callSite, f_t calleeMethod,
                                            n_t exitStmt, d_t exitNode,
                                            n_t reSite, d_t retNode) override {
    auto UserEF = IDEProblem.getReturnEdgeFunction(
        callSite, calleeMethod, exitStmt, exitNode, reSite, retNode);
    return std::make_shared<VarEdgeFunction<user_l_t>>(UserEF, TRUE_CONSTRAINT);
  }

  EdgeFunctionPtrType getCallToRetEdgeFunction(n_t callSite, d_t callNode,
                                               n_t retSite, d_t retSiteNode,
                                               std::set<f_t> callees) override {
    auto UserEF = IDEProblem.getCallToRetEdgeFunction(
        callSite, callNode, retSite, retSiteNode, callees);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Get User CTR EF: " << UserEF->str() << " AT "
                  << llvmIRToShortString(callSite));
    return std::make_shared<VarEdgeFunction<user_l_t>>(UserEF, TRUE_CONSTRAINT);
  }

  EdgeFunctionPtrType getSummaryEdgeFunction(n_t curr, d_t currNode, n_t succ,
                                             d_t succNode) override {
    return nullptr;
  }

  EdgeFunctionPtrType allTopFunction() override {
    auto Top = std::make_shared<AllTop<l_t>>(TOP);
    return Top;
  }

  l_t topElement() override { return TOP; }

  l_t bottomElement() override { return BOTTOM; }

  l_t join(l_t Lhs, l_t Rhs) override {
    // std::cout << "IDEVarTabulationProblem::join\n";
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

  d_t createZeroValue() const override {
    // ZeroValue should have been generated by the ctor of the original problem
    // IDEProblem.createZeroValue()
    return IDEProblem.getZeroValue();
  }

  bool isZeroValue(d_t d) const override { return IDEProblem.isZeroValue(d); }

  std::map<n_t, std::set<d_t>> initialSeeds() override {
    return IDEProblem.initialSeeds();
  }

  void printNode(std::ostream &os, n_t n) const override {
    IDEProblem.printNode(os, n);
  }

  void printDataFlowFact(std::ostream &os, d_t d) const override {
    IDEProblem.printDataFlowFact(os, d);
  }

  void printFunction(std::ostream &os, f_t fun) const override {
    IDEProblem.printFunction(os, fun);
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
IDEVarTabulationProblem(
    Problem &,
    VarICFG<typename Problem::ProblemAnalysisDomain::n_t,
            typename Problem::ProblemAnalysisDomain::f_t, z3::expr> &)
    -> IDEVarTabulationProblem<typename Problem::ProblemAnalysisDomain,
                               typename Problem::container_type>;

template <typename Problem>
using IDEVarTabulationProblem_P =
    IDEVarTabulationProblem<typename Problem::ProblemAnalysisDomain,
                            typename Problem::container_type>;

} // namespace psr

#endif
