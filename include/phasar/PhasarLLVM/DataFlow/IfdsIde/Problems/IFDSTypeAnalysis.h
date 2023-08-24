/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSTYPEANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSTYPEANALYSIS_H

#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"

#include <memory>
#include <set>
#include <string>

namespace psr {

class IFDSTypeAnalysis
    : public IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> {
public:
  IFDSTypeAnalysis(const LLVMProjectIRDB *IRDB,
                   std::vector<std::string> EntryPoints = {"main"});

  ~IFDSTypeAnalysis() override = default;

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t Curr, f_t DestFun) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const;

  [[nodiscard]] bool isZeroValue(d_t Fact) const noexcept override;
};
} // namespace psr

#endif
