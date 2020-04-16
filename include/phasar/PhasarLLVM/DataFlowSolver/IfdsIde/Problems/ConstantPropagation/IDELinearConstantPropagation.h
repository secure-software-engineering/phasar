#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ConstantPropagation/LCUtils/EdgeValueSet.h"

namespace CCPP {
/// \brief An implementation of a linear constant analysis, similar to
/// psr::IDELinearConstantAnalysis from phasar, but with an extended edge-value
/// domain. Instead of using single values, we use a bounded set of cadidates to
/// increase precision
class IDELinearConstantPropagation
    : public psr::LLVMDefaultIDETabulationProblem<
          const llvm::Value *, LCUtils::EdgeValueSet, psr::LLVMBasedICFG &> {
  size_t maxSetSize;

public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;

  typedef LCUtils::EdgeValueSet v_t;
  typedef psr::LLVMBasedICFG &i_t;
  struct LCAResult {
    LCAResult() = default;
    unsigned line_nr = 0;
    std::string src_code;
    std::map<std::string, v_t> variableToValue;
    std::vector<n_t> ir_trace;
    void print(std::ostream &os);
  };

  typedef std::map<std::string, std::map<unsigned, LCAResult>> lca_restults_t;

  IDELinearConstantPropagation(psr::LLVMBasedICFG &icfg,
                               const psr::LLVMTypeHierarchy &th,
                               const psr::ProjectIRDB &irdb, size_t maxSetSize);

  std::shared_ptr<psr::FlowFunction<d_t>>
  getNormalFlowFunction(n_t curr, n_t succ) override;

  std::shared_ptr<psr::FlowFunction<d_t>>
  getCallFlowFunction(n_t callStmt, m_t destMthd) override;

  std::shared_ptr<psr::FlowFunction<d_t>>
  getRetFlowFunction(n_t callSite, m_t calleeMthd, n_t exitStmt,
                     n_t retSite) override;

  std::shared_ptr<psr::FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<m_t> callees) override;

  std::shared_ptr<psr::FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, m_t destMthd) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() override;

  bool isZeroValue(d_t d) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<psr::EdgeFunction<v_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;

  std::shared_ptr<psr::EdgeFunction<v_t>>
  getCallEdgeFunction(n_t callStmt, d_t srcNode, m_t destinationMethod,
                      d_t destNode) override;

  std::shared_ptr<psr::EdgeFunction<v_t>>
  getReturnEdgeFunction(n_t callSite, m_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;

  std::shared_ptr<psr::EdgeFunction<v_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<m_t> callees) override;

  std::shared_ptr<psr::EdgeFunction<v_t>>
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override;

  v_t topElement() override;

  v_t bottomElement() override;

  v_t join(v_t lhs, v_t rhs) override;

  std::shared_ptr<psr::EdgeFunction<v_t>> allTopFunction() override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printMethod(std::ostream &os, m_t m) const override;

  void printValue(std::ostream &os, v_t v) const override;

  // void printIDEReport(std::ostream &os,
  // psr::SolverResults<n_t, d_t, v_t> &SR) override;
  void emitTextReport(std::ostream &os,
                      const psr::SolverResults<n_t, d_t, v_t> &SR) override;
  lca_restults_t getLCAResults(psr::SolverResults<n_t, d_t, v_t> SR);

private:
  void stripBottomResults(std::unordered_map<d_t, v_t> &res);
  bool isEntryPoint(const std::string &name) const;

public:
};
} // namespace CCPP