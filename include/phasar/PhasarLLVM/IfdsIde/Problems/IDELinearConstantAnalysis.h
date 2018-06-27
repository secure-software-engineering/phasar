/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/
#pragma once
#include <map>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/DefaultIDETabulationProblem.h>
#include <string>
#include <vector>

namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class IDELinearConstantAnalysis
    : public DefaultIDETabulationProblem<
          const llvm::Instruction *, const llvm::Value *,
          const llvm::Function *, int, LLVMBasedICFG &> {
private:
  std::vector<std::string> EntryPoints;

public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;
  typedef int v_t;
  typedef LLVMBasedICFG &i_t;

  static const int TOP;
  static const int BOTTOM;

  IDELinearConstantAnalysis(i_t icfg,
                            std::vector<std::string> EntryPoints = {"main"});

  virtual ~IDELinearConstantAnalysis() = default;

  // start formulating our analysis by specifying the parts required for IFDS

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t callStmt,
                                                         m_t destMthd) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t callSite,
                                                        m_t calleeMthd,
                                                        n_t exitStmt,
                                                        n_t retSite) override;

  std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<m_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, m_t destMthd) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() override;

  bool isZeroValue(d_t d) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<EdgeFunction<v_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;

  std::shared_ptr<EdgeFunction<v_t>> getCallEdgeFunction(n_t callStmt,
                                                         d_t srcNode,
                                                         m_t destiantionMethod,
                                                         d_t destNode) override;

  std::shared_ptr<EdgeFunction<v_t>>
  getReturnEdgeFunction(n_t callSite, m_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;

  std::shared_ptr<EdgeFunction<v_t>>
  getCallToReturnEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                              d_t retSiteNode) override;

  std::shared_ptr<EdgeFunction<v_t>>
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override;

  v_t topElement() override;

  v_t bottomElement() override;

  v_t join(v_t lhs, v_t rhs) override;

  std::shared_ptr<EdgeFunction<v_t>> allTopFunction() override;

  std::string DtoString(d_t d) const override;

  std::string VtoString(v_t v) const override;

  std::string NtoString(n_t n) const override;

  std::string MtoString(m_t m) const override;
};

} // namespace psr
