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

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>

namespace psr {

extern const std::shared_ptr<AllBottom<BinaryDomain>> ALL_BOTTOM;

/**
 * This class promotes a given IFDSTabulationProblem to an IDETabulationProblem
 * using a binary domain for the edge functions.
 */
template <typename N, typename D, typename F, typename T, typename V,
          typename I>
class IFDSToIDETabulationProblem
    : public IDETabulationProblem<N, D, F, T, V, BinaryDomain, I> {
public:
  IFDSTabulationProblem<N, D, F, T, V, I> &Problem;

  IFDSToIDETabulationProblem(
      IFDSTabulationProblem<N, D, F, T, V, I> &IFDSProblem)
      : IDETabulationProblem<N, D, F, T, V, BinaryDomain, I>(
            IFDSProblem.getProjectIRDB(), IFDSProblem.getTypeHierarchy(),
            IFDSProblem.getICFG(), IFDSProblem.getPointstoInfo(),
            IFDSProblem.getEntryPoints()),
        Problem(IFDSProblem) {
    this->ZeroValue = Problem.createZeroValue();
  }

  std::shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr,
                                                         N succ) override {
    return Problem.getNormalFlowFunction(curr, succ);
  }

  std::shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                       F destFun) override {
    return Problem.getCallFlowFunction(callStmt, destFun);
  }

  std::shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, F calleeFun, N exitStmt, N retSite) override {
    return Problem.getRetFlowFunction(callSite, calleeFun, exitStmt, retSite);
  }

  std::shared_ptr<FlowFunction<D>>
  getCallToRetFlowFunction(N callSite, N retSite,
                           std::set<F> callees) override {
    return Problem.getCallToRetFlowFunction(callSite, retSite, callees);
  }

  std::shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N callStmt,
                                                          F destFun) override {
    return Problem.getSummaryFlowFunction(callStmt, destFun);
  }

  std::map<N, std::set<D>> initialSeeds() override {
    return Problem.initialSeeds();
  }

  D createZeroValue() const override { return Problem.createZeroValue(); }

  bool isZeroValue(D d) const override { return Problem.isZeroValue(d); }

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
    if (Problem.isZeroValue(srcNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallEdgeFunction(N callStmt, D srcNode, F destinationFunction,
                      D destNode) override {
    if (Problem.isZeroValue(srcNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getReturnEdgeFunction(N callSite, F calleeFunction, N exitStmt, D exitNode,
                        N returnSite, D retNode) override {
    if (Problem.isZeroValue(exitNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::getInstance();
  }

  std::shared_ptr<EdgeFunction<BinaryDomain>>
  getCallToRetEdgeFunction(N callStmt, D callNode, N returnSite,
                           D returnSideNode, std::set<F> callees) override {
    if (Problem.isZeroValue(callNode))
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
    Problem.printNode(os, n);
  }

  void printDataFlowFact(std::ostream &os, D d) const override {
    Problem.printDataFlowFact(os, d);
  }

  void printFunction(std::ostream &os, F f) const override {
    Problem.printFunction(os, f);
  }

  void printEdgeFact(std::ostream &os, BinaryDomain v) const override {
    os << v;
  }
};

} // namespace psr

#endif
