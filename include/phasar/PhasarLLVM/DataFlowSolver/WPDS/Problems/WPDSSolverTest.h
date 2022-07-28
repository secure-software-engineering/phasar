/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_PROBLEMS_WPDSSOLVERTEST_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_WPDS_PROBLEMS_WPDSSOLVERTEST_H

#include "phasar/PhasarLLVM/DataFlowSolver/WPDS/WPDSProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
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

struct WPDSSolverTestAnalysisDomain : public LLVMAnalysisDomainDefault {
  using l_t = BinaryDomain;
};

class WPDSSolverTest : public WPDSProblem<WPDSSolverTestAnalysisDomain> {
public:
  WPDSSolverTest(const LLVMProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                 const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                 std::set<std::string> EntryPoints = {"main"});

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;
  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;
  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite) override;
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
  getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction, n_t ExitStmt,
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

  [[nodiscard]] d_t createZeroValue() const override;

  [[nodiscard]] bool isZeroValue(d_t Fact) const override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printNode(llvm::raw_ostream &OS, n_t Stmt) const override;
  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;
  void printFunction(llvm::raw_ostream &OS, f_t Func) const override;
  void printEdgeFact(llvm::raw_ostream &OS, l_t L) const override;
};

} // namespace psr

#endif
