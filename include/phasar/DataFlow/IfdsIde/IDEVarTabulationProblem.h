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

#include "phasar/ControlFlow/VarCFG.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/DataFlow/IfdsIde/VarEdgeFunctions.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/PhasarLLVM/VarStaticRenaming.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"

#include "llvm/Support/raw_ostream.h"

#include <map>
#include <memory>
#include <set>
#include <string>

#include <z3++.h>

namespace psr {

template <typename AnalysisDomainTy>
struct IDEVarProblemAnalysisDomainTransformer : public AnalysisDomainTy {
  // save user-problem l_t as underlying user_l_t
  using user_l_t = typename AnalysisDomainTy::l_t;
  // transform l_t to be a VarL<l_t>
  using l_t = VarL<typename AnalysisDomainTy::l_t>;
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

  using FlowFunctionPtrType =
      typename FlowFunctions<VarAnalysisDomainTy,
                             Container>::FlowFunctionPtrType;

  using EdgeFunctionPtrType = EdgeFunction<l_t>;

  IDEVarTabulationProblem(
      IDETabulationProblem<AnalysisDomainTy, Container> &IDEProblem,
      const i_t &ICF, const stringstringmap_t *StaticBackwardRenaming = nullptr)
      : IDETabulationProblem<VarAnalysisDomainTy, Container>(
            IDEProblem.getProjectIRDB(), IDEProblem.getEntryPoints(),
            IDEProblem.getZeroValue()),
        IDEProblem(IDEProblem), VarICF(ICF, StaticBackwardRenaming) {}

  // TODO also allow for solving IFDSTabulationProblems
  // IDEVarTabulationProblem(IFDSTabulationProblem ...) {}

  ~IDEVarTabulationProblem() override = default;

  // Flow functions
  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override {
    return IDEProblem.getNormalFlowFunction(Curr, Succ);
  }

  FlowFunctionPtrType getCallFlowFunction(n_t CallStmt, f_t DestMthd) override {
    return IDEProblem.getCallFlowFunction(CallStmt, DestMthd);
  }

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeMthd,
                                         n_t ExitStmt, n_t RetSite) override {
    return IDEProblem.getRetFlowFunction(CallSite, CalleeMthd, ExitStmt,
                                         RetSite);
  }

  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override {
    return IDEProblem.getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  FlowFunctionPtrType getSummaryFlowFunction(n_t Curr, f_t DestMthd) override {
    return IDEProblem.getSummaryFlowFunction(Curr, DestMthd);
  }

  // Edge functions
  EdgeFunctionPtrType getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                            d_t SuccNode) override {
    auto UserEF =
        IDEProblem.getNormalEdgeFunction(Curr, CurrNode, Succ, SuccNode);
    // if curr is a special preprocessor #ifdef instruction, we need to add a
    // preprocessor constraint
    if (VarICF.isPPBranchTarget(Curr, Succ)) {
      return VarEdgeFunction<user_l_t>(
          std::move(UserEF), VarICF.getPPConstraintOrTrue(Curr, Succ));
    }

    // ordinary instruction, no preprocessor constraints
    return VarEdgeFunction<user_l_t>::from(std::move(UserEF), TrueConstraint);
  }

  EdgeFunctionPtrType getCallEdgeFunction(n_t CallStmt, d_t SrcNode,
                                          f_t DestinationMethod,
                                          d_t DestNode) override {
    auto UserEF = IDEProblem.getCallEdgeFunction(CallStmt, SrcNode,
                                                 DestinationMethod, DestNode);
    return VarEdgeFunction<user_l_t>::from(std::move(UserEF), TrueConstraint);
  }

  EdgeFunctionPtrType getReturnEdgeFunction(n_t CallSite, f_t CalleeMethod,
                                            n_t ExitStmt, d_t ExitNode,
                                            n_t ReSite, d_t RetNode) override {
    auto UserEF = IDEProblem.getReturnEdgeFunction(
        CallSite, CalleeMethod, ExitStmt, ExitNode, ReSite, RetNode);
    return VarEdgeFunction<user_l_t>::from(std::move(UserEF), TrueConstraint);
  }

  EdgeFunctionPtrType
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override {
    auto UserEF = IDEProblem.getCallToRetEdgeFunction(
        CallSite, CallNode, RetSite, RetSiteNode, Callees);
    PHASAR_LOG_LEVEL(DEBUG,
                     "Get User CTR EF: " << UserEF << " AT "
                                         << llvmIRToShortString(CallSite));
    return VarEdgeFunction<user_l_t>::from(std::move(UserEF), TrueConstraint);
  }

  EdgeFunctionPtrType getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                             d_t SuccNode) override {
    auto UserEF =
        IDEProblem.getSummaryEdgeFunction(Curr, CurrNode, Succ, SuccNode);
    if (!UserEF) {
      return nullptr;
    }

    return VarEdgeFunction<user_l_t>::from(std::move(UserEF), TrueConstraint);
  }

  EdgeFunctionPtrType allTopFunction() override { return AllTop<l_t>{Top}; }

  l_t topElement() override { return Top; }

  l_t bottomElement() override { return Bottom; }

  l_t join(l_t Lhs, l_t Rhs) override {
    // std::cout << "IDEVarTabulationProblem::join\n";
    // std::cout << "lhs: ";
    // printEdgeFact(std::cout, Lhs);
    // std::cout << "rhs: ";
    // printEdgeFact(std::cout, Rhs);
    // std::cout << " --> ";
    for (auto &[LConstraint, LValue] : Lhs) {
      auto [It, Inserted] = Rhs.try_emplace(LConstraint, std::move(LValue));
      if (!Inserted) {
        It->second = IDEProblem.join(std::move(It->second), std::move(LValue));
      }
    }
    // printEdgeFact(std::cout, Rhs);
    // std::cout << '\n';
    return Rhs;
  }

  d_t createZeroValue() const {
    // ZeroValue should have been generated by the ctor of the original problem
    // IDEProblem.createZeroValue()
    return IDEProblem.getZeroValue();
  }

  bool isZeroValue(d_t d) const noexcept override {
    return IDEProblem.isZeroValue(d);
  }

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    typename InitialSeeds<n_t, d_t, l_t>::GeneralizedSeeds Ret;

    auto UserSeeds = IDEProblem.initialSeeds();
    for (auto &[Inst, Facts] : UserSeeds.getSeeds()) {
      auto &AtInst = Ret[Inst];
      for (auto &[Fact, Val] : Facts) {
        AtInst[Fact] = {{TrueConstraint, std::move(Val)}};
      }
    }

    return std::move(Ret);
  }

private:
  IDETabulationProblem<AnalysisDomainTy, Container> &IDEProblem;
  VarCFG<i_t, z3::expr> VarICF;

  z3::expr TrueConstraint = VarICF.getTrueConstraint();
  l_t Bottom = {{TrueConstraint, IDEProblem.bottomElement()}};
  l_t Top = {{}};
};

template <typename UserL>
[[nodiscard]] inline std::string LToString(const VarL<UserL> &LatticeVal) {
  std::string Ret;
  llvm::raw_string_ostream OS(Ret);

  for (const auto &[Constraint, Value] : LatticeVal) {
    OS << '<' << Constraint << ", " << LToString(Value) << ">; ";
  }
  return Ret;
}

// template <typename Problem>
// IDEVarTabulationProblem(
//     Problem &,
//     VarICFG<typename Problem::ProblemAnalysisDomain::n_t,
//             typename Problem::ProblemAnalysisDomain::f_t, z3::expr> &)
//     -> IDEVarTabulationProblem<typename Problem::ProblemAnalysisDomain,
//                                typename Problem::container_type>;

// template <typename Problem>
// using IDEVarTabulationProblem_P =
//     IDEVarTabulationProblem<typename Problem::ProblemAnalysisDomain,
//                             typename Problem::container_type>;

} // namespace psr

#endif
