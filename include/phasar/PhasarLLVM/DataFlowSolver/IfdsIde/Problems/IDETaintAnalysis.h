/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDETAINTANALYSIS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDETAINTANALYSIS_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

namespace llvm {
class Instruction;
class Function;
class StructType;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

struct IDETaintAnalysisDomain : LLVMAnalysisDomainDefault {
  using l_t = const llvm::Value *;
};

class IDETaintAnalysis : public IDETabulationProblem<IDETaintAnalysisDomain> {
public:
  using IDETabProblemType = IDETabulationProblem<IDETaintAnalysisDomain>;
  using typename IDETabProblemType::d_t;
  using typename IDETabProblemType::f_t;
  using typename IDETabProblemType::i_t;
  using typename IDETabProblemType::l_t;
  using typename IDETabProblemType::n_t;
  using typename IDETabProblemType::t_t;
  using typename IDETabProblemType::v_t;

  std::set<std::string> source_functions = {"fread", "read"};
  // keep in mind that 'char** argv' of main is a source for tainted values as
  // well
  std::set<std::string> sink_functions = {"fwrite", "write", "printf"};
  static bool setContainsStr(std::set<std::string> s, const std::string &str);

  IDETaintAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                   const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                   std::set<std::string> EntryPoints = {"main"});

  ~IDETaintAnalysis() override = default;

  // start formulating our analysis by specifying the parts required for IFDS

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t callStmt, f_t destFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeFun,
                                         n_t exitStmt, n_t retSite) override;

  FlowFunctionPtrType getCallToRetFlowFunction(n_t callSite, n_t retSite,
                                               std::set<f_t> callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t callStmt,
                                             f_t destFun) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() const override;

  bool isZeroValue(d_t d) const override;

  // in addition provide specifications for the IDE parts

  EdgeFunctionPtrType
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;

  EdgeFunctionPtrType
  getCallEdgeFunction(n_t callStmt, d_t srcNode, f_t destinationFunction,
                      d_t destNode) override;

  EdgeFunctionPtrType
  getReturnEdgeFunction(n_t callSite, f_t calleeFunction, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;

  EdgeFunctionPtrType
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) override;

  EdgeFunctionPtrType
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t lhs, l_t rhs) override;

  EdgeFunctionPtrType allTopFunction() override;

  class IDETainAnalysisAllTop
      : public EdgeFunction<l_t> {
    l_t computeTarget(l_t source) override;

    EdgeFunctionPtrType
    composeWith(EdgeFunctionPtrType secondFunction, MemoryManager<AnalysisDomainTy, Container> memManager) override;

    EdgeFunctionPtrType
    joinWith(EdgeFunctionPtrType otherFunction) override;

    bool equal_to(EdgeFunctionPtrType other) const override;
  };

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printNode(std::ostream &os, n_t n) const override;

  void printFunction(std::ostream &os, f_t m) const override;

  void printEdgeFact(std::ostream &os, l_t V) const override;
};

} // namespace psr

#endif
