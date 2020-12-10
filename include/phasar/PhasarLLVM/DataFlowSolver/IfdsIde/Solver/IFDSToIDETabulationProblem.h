/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IFDSTOIDETABULATIONPROBLEM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_IFDSTOIDETABULATIONPROBLEM_H_

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
struct is_analysis_domain_extensions : std::false_type {};

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

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override {
    return Problem.getNormalFlowFunction(curr, succ);
  }

  FlowFunctionPtrType getCallFlowFunction(n_t callStmt, f_t destFun) override {
    return Problem.getCallFlowFunction(callStmt, destFun);
  }

  FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeFun,
                                         n_t exitStmt, n_t retSite) override {
    return Problem.getRetFlowFunction(callSite, calleeFun, exitStmt, retSite);
  }

  FlowFunctionPtrType getCallToRetFlowFunction(n_t callSite, n_t retSite,
                                               std::set<f_t> callees) override {
    return Problem.getCallToRetFlowFunction(callSite, retSite, callees);
  }

  FlowFunctionPtrType getSummaryFlowFunction(n_t callStmt,
                                             f_t destFun) override {
    return Problem.getSummaryFlowFunction(callStmt, destFun);
  }

  std::map<n_t, std::set<d_t>> initialSeeds() override {
    return Problem.initialSeeds();
  }

  d_t createZeroValue() const override { return Problem.createZeroValue(); }

  bool isZeroValue(d_t d) const override { return Problem.isZeroValue(d); }

  BinaryDomain topElement() override { return BinaryDomain::TOP; }

  BinaryDomain bottomElement() override { return BinaryDomain::BOTTOM; }

  BinaryDomain join(BinaryDomain left, BinaryDomain right) override {
    if (left == BinaryDomain::TOP && right == BinaryDomain::TOP) {
      return BinaryDomain::TOP;
    } else {
      return BinaryDomain::BOTTOM;
    }
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>> allTopFunction() override {
    return std::make_shared<AllTop<BinaryDomain>>(BinaryDomain::TOP);
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getNormalEdgeFunction(n_t src, d_t srcNode, n_t tgt, d_t tgtNode) override {
    if (Problem.isZeroValue(srcNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallEdgeFunction(n_t callStmt, d_t srcNode, f_t destinationFunction,
                      d_t destNode) override {
    if (Problem.isZeroValue(srcNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getReturnEdgeFunction(n_t callSite, f_t calleeFunction, n_t exitStmt,
                        d_t exitNode, n_t returnSite, d_t retNode) override {
    if (Problem.isZeroValue(exitNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallToRetEdgeFunction(n_t callStmt, d_t callNode, n_t returnSite,
                           d_t returnSideNode, std::set<f_t> callees) override {
    if (Problem.isZeroValue(callNode)) {
      return ALLBOTTOM;
    }
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
  }

  void printNode(std::ostream &os, n_t n) const override {
    Problem.printNode(os, n);
  }

  void printDataFlowFact(std::ostream &os, d_t d) const override {
    Problem.printDataFlowFact(os, d);
  }

  void printFunction(std::ostream &os, f_t f) const override {
    Problem.printFunction(os, f);
  }

  void printEdgeFact(std::ostream &os, BinaryDomain v) const override {
    os << v;
  }
};

} // namespace psr

#endif
