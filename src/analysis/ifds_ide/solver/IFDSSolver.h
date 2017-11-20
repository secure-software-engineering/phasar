/*
 * IFDSSolver.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_SOLVER_IFDSSOLVER_H_
#define ANALYSIS_IFDS_IDE_SOLVER_IFDSSOLVER_H_

#include "../../misc/BinaryDomain.h"
#include "../EdgeFunction.h"
#include "../IDETabulationProblem.h"
#include "../IFDSTabulationProblem.h"
#include "../edge_func/AllBottom.h"
#include "../edge_func/AllTop.h"
#include "../edge_func/EdgeIdentity.h"
#include "IDESolver.h"
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>

using namespace std;

extern const shared_ptr<AllBottom<BinaryDomain>> ALL_BOTTOM;

/**
 * We have to find an elegant way to promote an IFDSTabulationProblem to an
 * IDETabulationProblem!!!
 * This class override the function in order to promote an IFDSTabulationProblem
 * to an IDETabulationProblem using
 * a binary lattice.
 */
template <class N, class D, class M, class I>
class IFDSToIDETabulationProblem
    : public IDETabulationProblem<N, D, M, BinaryDomain, I> {
public:
  IFDSTabulationProblem<N, D, M, I> &problem;

  IFDSToIDETabulationProblem(IFDSTabulationProblem<N, D, M, I> &ifdsProblem)
      : problem(ifdsProblem) {
    this->solver_config.followReturnsPastSeeds =
        problem.solver_config.followReturnsPastSeeds;
    this->solver_config.autoAddZero = problem.solver_config.autoAddZero;
    this->solver_config.computeValues = problem.solver_config.computeValues;
    this->solver_config.recordEdges = problem.solver_config.recordEdges;
    this->solver_config.computePersistedSummaries =
        problem.solver_config.computePersistedSummaries;
  }

  shared_ptr<FlowFunction<D>> getNormalFlowFunction(N curr, N succ) override {
    return problem.getNormalFlowFunction(curr, succ);
  }

  shared_ptr<FlowFunction<D>> getCallFlowFunction(N callStmt,
                                                  M destMthd) override {
    return problem.getCallFlowFunction(callStmt, destMthd);
  }

  shared_ptr<FlowFunction<D>>
  getRetFlowFunction(N callSite, M calleeMthd, N exitStmt, N retSite) override {
    return problem.getRetFlowFunction(callSite, calleeMthd, exitStmt, retSite);
  }

  shared_ptr<FlowFunction<D>> getCallToRetFlowFunction(N callSite,
                                                       N retSite) override {
    return problem.getCallToRetFlowFunction(callSite, retSite);
  }

  shared_ptr<FlowFunction<D>> getSummaryFlowFunction(N callStmt,
                                                     M destMthd) override {
    return problem.getSummaryFlowFunction(callStmt, destMthd);
  }

  I interproceduralCFG() override { return problem.interproceduralCFG(); }

  map<N, set<D>> initialSeeds() override { return problem.initialSeeds(); }

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

  shared_ptr<EdgeFunction<BinaryDomain>> allTopFunction() override {
    return make_shared<AllTop<BinaryDomain>>(BinaryDomain::TOP);
  }

  shared_ptr<EdgeFunction<BinaryDomain>>
  getNormalEdgeFunction(N src, D srcNode, N tgt, D tgtNode) override {
    if (problem.isZeroValue(srcNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::v();
  }

  shared_ptr<EdgeFunction<BinaryDomain>>
  getCallEdgeFunction(N callStmt, D srcNode, M destinationMethod,
                      D destNode) override {
    if (problem.isZeroValue(srcNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::v();
  }

  shared_ptr<EdgeFunction<BinaryDomain>>
  getReturnEdgeFunction(N callSite, M calleeMethod, N exitStmt, D exitNode,
                        N returnSite, D retNode) override {
    if (problem.isZeroValue(exitNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::v();
  }

  shared_ptr<EdgeFunction<BinaryDomain>>
  getCallToReturnEdgeFunction(N callStmt, D callNode, N returnSite,
                              D returnSideNode) override {
    if (problem.isZeroValue(callNode))
      return ALL_BOTTOM;
    else
      return EdgeIdentity<BinaryDomain>::v();
  }

  shared_ptr<EdgeFunction<BinaryDomain>>
  getSummaryEdgeFunction(N callStmt, D callNode, N retSite,
                         D retSiteNode) override {
    // if(problem.isZeroValue(srcNode))
    // return ALL_BOTTOM;
    // else
    return EdgeIdentity<BinaryDomain>::v();
  }

  string DtoString(D d) override { return problem.DtoString(d); }

  string VtoString(BinaryDomain v) override {
    ostringstream osst;
    osst << v;
    return osst.str();
  }

  string MtoString(M m) override { return problem.MtoString(m); }

  string NtoString(N n) override { return problem.NtoString(n); }
};

template <typename N, typename D, typename M, typename I>
class IFDSSolver : public IDESolver<N, D, M, BinaryDomain, I> {
public:
  IFDSSolver(IFDSTabulationProblem<N, D, M, I> &ifdsProblem)
      : // caution, this calls IDESolver's move constructor, because
        // IFDSToIDETabulationProblem is a temporary object!
        IDESolver<N, D, M, BinaryDomain, I>(
            IFDSToIDETabulationProblem<N, D, M, I>(ifdsProblem)) {}

  virtual ~IFDSSolver() = default;

  set<D> ifdsResultsAt(N stmt) {
    set<D> keyset;
    map<D, BinaryDomain> map = resultsAt(stmt);
    for_each(map.begin(), map.end(), [&](D d) { keyset.insert(d.first); });
    return keyset;
  }
};

#endif /* ANALYSIS_IFDS_IDE_SOLVER_IFDSSOLVER_HH_ */
