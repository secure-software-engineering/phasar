/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_PROBLEMS_WPDSALIASCOLLECTOR_H_
#define PHASAR_PHASARLLVM_WPDS_PROBLEMS_WPDSALIASCOLLECTOR_H_

#include <memory>

#include <phasar/PhasarLLVM/Utils/BinaryDomain.h>
#include <phasar/PhasarLLVM/Utils/Printer.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/LLVMDefaultWPDSProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSOptions.h>

namespace llvm {
class Instruction;
class Value;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class ProjectIRDB;

class WPDSAliasCollector
    : public LLVMDefaultWPDSProblem<const llvm::Value *, BinaryDomain,
                                    LLVMBasedICFG &> {
public:
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *m_t;
  typedef BinaryDomain v_t;
  typedef LLVMBasedICFG &i_t;

  WPDSAliasCollector(LLVMBasedICFG &I, const LLVMTypeHierarchy &TH,
                     const ProjectIRDB &DB, WPDSType WPDS,
                     SearchDirection Direction, std::vector<n_t> Stack = {},
                     bool Witnesses = false);

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
  getSummaryFlowFunction(n_t curr, m_t destMthd) override;

  std::shared_ptr<EdgeFunction<v_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;
  std::shared_ptr<EdgeFunction<v_t>> getCallEdgeFunction(n_t callStmt,
                                                         d_t srcNode,
                                                         m_t destinationMethod,
                                                         d_t destNode) override;
  std::shared_ptr<EdgeFunction<v_t>>
  getReturnEdgeFunction(n_t callSite, m_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;
  std::shared_ptr<EdgeFunction<v_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<m_t> callees) override;
  std::shared_ptr<EdgeFunction<v_t>>
  getSummaryEdgeFunction(n_t curr, d_t currNode, n_t succ,
                         d_t succNode) override;

  v_t topElement() override;
  v_t bottomElement() override;
  v_t join(v_t lhs, v_t rhs) override;

  d_t zeroValue() override;

  bool isZeroValue(WPDSAliasCollector::d_t d) const override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  std::shared_ptr<EdgeFunction<v_t>> allTopFunction() override;

  void printNode(std::ostream &os, n_t n) const override;
  void printDataFlowFact(std::ostream &os, d_t d) const override;
  void printMethod(std::ostream &os, m_t m) const override;
  void printValue(std::ostream &os, v_t v) const override;
};

} // namespace psr

#endif
