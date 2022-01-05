/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IFDSTOIDETABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_IFDSTOIDETABULATIONPROBLEM_H

#include <memory>
#include <set>
#include <sstream>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"

namespace psr {

extern const std::shared_ptr<AllBottom<BinaryDomain>> ALLBOTTOM;

template <typename OriginalAnalysisDomain>
struct AnalysisDomainExtender : public OriginalAnalysisDomain {
  static_assert(
      std::is_same<void, typename OriginalAnalysisDomain::l_t>::value ||
          std::is_same<BinaryDomain,
                       typename OriginalAnalysisDomain::l_t>::value,
      "Lattice domain is overwritten here, expected it to be void or "
      "BinaryDomain but found a different type.");
  using BaseAnalysisDomain = OriginalAnalysisDomain;
  using l_t = BinaryDomain;
};

template <typename, typename = void>
struct is_analysis_domain_extensions : std::false_type {}; // NOLINT

template <typename AnalysisDomainTy>
struct is_analysis_domain_extensions<
    AnalysisDomainTy,
    std::void_t<typename AnalysisDomainTy::BaseAnalysisDomain>>
    : std::true_type {};

/**
 * This class promotes a given IFDSTabulationProblem to an IDETabulationProblem
 * using a binary domain for the edge functions.
 */
template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IFDSToIDETabulationProblem
    : public IDETabulationProblem<AnalysisDomainExtender<AnalysisDomainTy>,
                                  Container> {
  using typename IDETabulationProblem<AnalysisDomainExtender<AnalysisDomainTy>,
                                      Container>::FlowFunctionPtrType;

  using n_t = typename AnalysisDomainExtender<AnalysisDomainTy>::n_t;
  using f_t = typename AnalysisDomainExtender<AnalysisDomainTy>::f_t;
  using d_t = typename AnalysisDomainExtender<AnalysisDomainTy>::d_t;
  using l_t = typename AnalysisDomainExtender<AnalysisDomainTy>::l_t;

public:
  IFDSTabulationProblem<AnalysisDomainTy, Container> &Problem;

  IFDSToIDETabulationProblem(
      IFDSTabulationProblem<AnalysisDomainTy> &IFDSProblem)
      : IDETabulationProblem<AnalysisDomainExtender<AnalysisDomainTy>>(
            IFDSProblem.getProjectIRDB(), IFDSProblem.getTypeHierarchy(),
            IFDSProblem.getICFG(), IFDSProblem.getPointstoInfo(),
            IFDSProblem.getEntryPoints()),
        Problem(IFDSProblem) {
    this->ZeroValue = Problem.createZeroValue();
  }

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override {
    return Problem.getNormalFlowFunction(Curr, Succ);
  }

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override {
    return Problem.getCallFlowFunction(CallSite, DestFun);
  }

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitInst, n_t RetSite) override {
    return Problem.getRetFlowFunction(CallSite, CalleeFun, ExitInst, RetSite);
  }

  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               std::set<f_t> Callees) override {
    return Problem.getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestFun) override {
    return Problem.getSummaryFlowFunction(CallSite, DestFun);
  }

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return Problem.initialSeeds();
  }

  [[nodiscard]] d_t createZeroValue() const override {
    return Problem.createZeroValue();
  }

  [[nodiscard]] bool isZeroValue(d_t Fact) const override {
    return Problem.isZeroValue(Fact);
  }

  BinaryDomain topElement() override { return BinaryDomain::TOP; }

  BinaryDomain bottomElement() override { return BinaryDomain::BOTTOM; }

  BinaryDomain join(BinaryDomain Lhs, BinaryDomain Rhs) override {
    if (Lhs == BinaryDomain::TOP && Rhs == BinaryDomain::TOP) {
      return BinaryDomain::TOP;
    }
    return BinaryDomain::BOTTOM;
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>> allTopFunction() override {
    return std::make_shared<AllTop<BinaryDomain>>(BinaryDomain::TOP);
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getNormalEdgeFunction(n_t /*Src*/, d_t SrcNode, n_t /*Tgt*/,
                        d_t /*TgtNode*/) override {
    if (Problem.isZeroValue(SrcNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallEdgeFunction(n_t /*CallSite*/, d_t SrcNode,
                      f_t /*DestinationFunction*/, d_t /*DestNode*/) override {
    if (Problem.isZeroValue(SrcNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getReturnEdgeFunction(n_t /*CallSite*/, f_t /*CalleeFunction*/,
                        n_t /*ExitInst*/, d_t ExitNode, n_t /*ReturnSite*/,
                        d_t /*RetNode*/) override {
    if (Problem.isZeroValue(ExitNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallToRetEdgeFunction(n_t /*CallSite*/, d_t CallNode, n_t /*ReturnSite*/,
                           d_t /*ReturnSideNode*/,
                           std::set<f_t> /*Callees*/) override {
    if (Problem.isZeroValue(CallNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getSummaryEdgeFunction(n_t /*CallSite*/, d_t /*CallNode*/, n_t /*RetSite*/,
                         d_t /*RetSiteNode*/) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  void printNode(std::ostream &OS, n_t Stmt) const override {
    Problem.printNode(OS, Stmt);
  }

  void printDataFlowFact(std::ostream &OS, d_t Fact) const override {
    Problem.printDataFlowFact(OS, Fact);
  }

  void printFunction(std::ostream &OS, f_t Func) const override {
    Problem.printFunction(OS, Func);
  }

  void printEdgeFact(std::ostream &OS, BinaryDomain Val) const override {
    OS << Val;
  }

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &Results,
                      std::ostream &OS = std::cout) override {
    Problem.emitTextReport(Results, OS);
  }

  void emitGraphicalReport(const SolverResults<n_t, d_t, l_t> &Results,
                           std::ostream &OS = std::cout) override {
    Problem.emitGraphicalReport(Results, OS);
  }
};

} // namespace psr

#endif
