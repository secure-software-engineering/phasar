/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_IDEGENERALIZEDLCA_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_IDEGENERALIZEDLCA_H_

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"

namespace psr {
/// \brief An implementation of a linear constant analysis, similar to
/// IDELinearConstantAnalysis, but with an extended edge-value
/// domain. Instead of using single values, we use a bounded set of cadidates to
/// increase precision.

struct IDEGeneralizedLCADomain : LLVMAnalysisDomainDefault {
  using l_t = EdgeValueSet;
};

// Forward declare the IDETabulationProblem as we require its toString
// functionality.
template <typename AnalysisDomainTy, typename Container>
class IDETabulationProblem;

class IDEGeneralizedLCA : public IDETabulationProblem<IDEGeneralizedLCADomain> {
  size_t maxSetSize;

public:
  using d_t = typename IDEGeneralizedLCADomain::d_t;
  using f_t = typename IDEGeneralizedLCADomain::f_t;
  using i_t = typename IDEGeneralizedLCADomain::i_t;
  using l_t = typename IDEGeneralizedLCADomain::l_t;
  using n_t = typename IDEGeneralizedLCADomain::n_t;
  using t_t = typename IDEGeneralizedLCADomain::t_t;
  using v_t = typename IDEGeneralizedLCADomain::v_t;

  struct LCAResult {
    LCAResult() = default;
    unsigned line_nr = 0;
    std::string src_code;
    std::map<std::string, l_t> variableToValue;
    std::vector<n_t> ir_trace;
    void print(std::ostream &os);
  };

  using lca_results_t = std::map<std::string, std::map<unsigned, LCAResult>>;

  IDEGeneralizedLCA(
      const ProjectIRDB *IRDB,
      const TypeHierarchy<const llvm::StructType *, const llvm::Function *> *TH,
      const LLVMBasedICFG *ICF,
      PointsToInfo<const llvm::Value *, const llvm::Instruction *> *PT,
      std::set<std::string> EntryPoints, size_t MaxSetSize);

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t callStmt,
                                                         f_t destMthd) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t callSite,
                                                        f_t calleeMthd,
                                                        n_t exitStmt,
                                                        n_t retSite) override;

  std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<f_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, f_t destMthd) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() const override;

  bool isZeroValue(d_t d) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;

  std::shared_ptr<EdgeFunction<l_t>> getCallEdgeFunction(n_t callStmt,
                                                         d_t srcNode,
                                                         f_t destinationMethod,
                                                         d_t destNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t callSite, f_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t lhs, l_t rhs) override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, f_t m) const override;

  void printEdgeFact(std::ostream &os, l_t v) const override;

  // void printIDEReport(std::ostream &os,
  // SolverResults<n_t, d_t, l_t> &SR) override;
  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      std::ostream &os) override;

  lca_results_t getLCAResults(SolverResults<n_t, d_t, l_t> SR);

private:
  void stripBottomResults(std::unordered_map<d_t, l_t> &res);
  bool isEntryPoint(const std::string &name) const;
  template <typename V> std::string VtoString(V v);
  bool isStringConstructor(const llvm::Function *F);
};

} // namespace psr

#endif
