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

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllBottom.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllTop.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>

namespace psr {

extern const std::shared_ptr<AllBottom<BinaryDomain>> ALL_BOTTOM;

/**
 * This class promotes a given IFDSTabulationProblem to an IDETabulationProblem
 * using a binary domain for the edge functions.
 */
template <typename N, typename D, typename M, typename I>
class IFDSToIDETabulationProblem
    : public IDETabulationProblem<N, D, M, BinaryDomain, I> {
public:
  IFDSTabulationProblem<N, D, M, I> &problem;

  IFDSToIDETabulationProblem(IFDSTabulationProblem<N, D, M, I> &ifdsProblem)
      : IDETabulationProblem<N, D, M, BinaryDomain, I>(), problem(ifdsProblem) {
    // std::cout << "IFDSToIDETabulationProblem::IFDSToIDETabulationProblem()"
    // << std::endl;
    this->solver_config = problem.getSolverConfiguration();
  }

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

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N callStmt,
                                                          M destMthd) override {
    return problem.getSummaryFlowFunction(callStmt, destMthd);
  }

  I interproceduralCFG() override { return problem.interproceduralCFG(); }

  std::map<N, std::set<D>> initialSeeds() override {
    return problem.initialSeeds();
  }

  D zeroValue() override { return problem.zeroValue(); }

  bool isZeroValue(D d) const override { return problem.isZeroValue(d); }

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
  getNormalEdgeFunction(N src, D srcNode, N tgt, D tgtNode) override {
    if (problem.isZeroValue(srcNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallEdgeFunction(N callStmt, D srcNode, M destinationMethod,
                      D destNode) override {
    if (problem.isZeroValue(srcNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getReturnEdgeFunction(N callSite, M calleeMethod, N exitStmt, D exitNode,
                        N returnSite, D retNode) override {
    if (problem.isZeroValue(exitNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallToRetEdgeFunction(N callStmt, D callNode, N returnSite,
                           D returnSideNode, std::set<M> callees) override {
    if (problem.isZeroValue(callNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getSummaryEdgeFunction(N callStmt, D callNode, N retSite,
                         D retSiteNode) override {
    return EdgeIdentity<BinaryDomain>::getInstance();
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

  void printValue(std::ostream &os, BinaryDomain v) const override { os << v; }
};

} // namespace psr

#endif
