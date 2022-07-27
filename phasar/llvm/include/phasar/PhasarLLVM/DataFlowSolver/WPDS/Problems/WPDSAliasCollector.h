/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_PROBLEMS_WPDSALIASCOLLECTOR_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_PROBLEMS_WPDSALIASCOLLECTOR_H

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

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;
  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;
  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitInst, n_t RetSite) override;
  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               std::set<f_t> Callees) override;
  FlowFunctionPtrType getSummaryFlowFunction(n_t Curr, f_t DestFun) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                        d_t SuccNode) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getCallEdgeFunction(n_t CallSite, d_t SrcNode, f_t DestinationFunction,
                      d_t DestNode) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction, n_t ExitInst,
                        d_t ExitNode, n_t RetSite, d_t RetNode) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode, std::set<f_t> Callees) override;
  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                         d_t SuccNode) override;

  l_t topElement() override;
  l_t bottomElement() override;
  l_t join(l_t Lhs, l_t Rhs) override;

  bool isZeroValue(WPDSAliasCollector::d_t Fact) const override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printNode(llvm::raw_ostream &OS, n_t Stmt) const override;
  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;
  void printFunction(llvm::raw_ostream &OS, f_t Func) const override;
  void printEdgeFact(llvm::raw_ostream &OS, l_t L) const override;
};

} // namespace psr

#endif
