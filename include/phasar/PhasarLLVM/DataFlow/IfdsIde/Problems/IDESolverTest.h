/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDESOLVERTEST_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IDESOLVERTEST_H

#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"

#include <set>
#include <string>
#include <vector>

namespace llvm {
class Instruction;
class Function;
class StructType;
class Value;
} // namespace llvm

namespace psr {

struct IDESolverTestAnalysisDomain : public LLVMAnalysisDomainDefault {
  using l_t = const llvm::Value *;
};

class IDESolverTest : public IDETabulationProblem<IDESolverTestAnalysisDomain> {

public:
  using IDETabProblemType = IDETabulationProblem<IDESolverTestAnalysisDomain>;
  using typename IDETabProblemType::d_t;
  using typename IDETabProblemType::f_t;
  using typename IDETabProblemType::i_t;
  using typename IDETabProblemType::l_t;
  using typename IDETabProblemType::n_t;
  using typename IDETabProblemType::t_t;
  using typename IDETabProblemType::v_t;

  IDESolverTest(const LLVMProjectIRDB *IRDB,
                std::vector<std::string> EntryPoints = {"main"});

  ~IDESolverTest() override = default;

  // start formulating our analysis by specifying the parts required for IFDS

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestFun) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const;

  [[nodiscard]] bool isZeroValue(d_t Fact) const noexcept override;

  // in addition provide specifications for the IDE parts

  EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode) override;

  EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                        f_t DestinationFunction,
                                        d_t DestNode) override;

  EdgeFunction<l_t> getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                          n_t ExitStmt, d_t ExitNode,
                                          n_t RetSite, d_t RetNode) override;

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override;

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t CallSite, d_t CallNode,
                                           n_t RetSite,
                                           d_t RetSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t Lhs, l_t Rhs) override;

  EdgeFunction<l_t> allTopFunction() override;
};

} // namespace psr

#endif
