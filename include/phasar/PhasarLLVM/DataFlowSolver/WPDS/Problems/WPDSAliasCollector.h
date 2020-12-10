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
#include <set>
#include <string>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/BinaryDomain.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"

namespace llvm {
class Instruction;
class Value;
class StructType;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMPointsToInfo;
class LLVMTypeHierarchy;
class ProjectIRDB;

struct WPDSAliasCollectorAnalysisDomain : public LLVMAnalysisDomainDefault {
  using l_t = BinaryDomain;
};

class WPDSAliasCollector
    : public WPDSProblem<WPDSAliasCollectorAnalysisDomain> {
public:
  WPDSAliasCollector(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                     const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                     std::set<std::string> EntryPoints);

  ~WPDSAliasCollector() override = default;

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override;
  FlowFunctionPtrType getCallFlowFunction(n_t callStmt, f_t destFun) override;
  FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeFun,
                                         n_t exitStmt, n_t retSite) override;
  FlowFunctionPtrType getCallToRetFlowFunction(n_t callSite, n_t retSite,
                                               std::set<f_t> callees) override;
  FlowFunctionPtrType getSummaryFlowFunction(n_t curr, f_t destFun) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getCallEdgeFunction(n_t callStmt, d_t srcNode, f_t destinationFunction,
                      d_t destNode) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t callSite, f_t calleeFunction, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t curr, d_t currNode, n_t succ,
                         d_t succNode) override;

  l_t topElement() override;
  l_t bottomElement() override;
  l_t join(l_t lhs, l_t rhs) override;

  bool isZeroValue(WPDSAliasCollector::d_t d) const override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printNode(std::ostream &os, n_t n) const override;
  void printDataFlowFact(std::ostream &os, d_t d) const override;
  void printFunction(std::ostream &os, f_t m) const override;
  void printEdgeFact(std::ostream &os, l_t v) const override;
};

} // namespace psr

#endif
