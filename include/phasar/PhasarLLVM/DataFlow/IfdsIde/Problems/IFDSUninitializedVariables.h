/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSUNINITIALIZEDVARIABLES_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSUNINITIALIZEDVARIABLES_H

#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace psr {

class IFDSUninitializedVariables
    : public IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> {
  struct UninitResult {
    UninitResult() = default;
    unsigned int Line = 0;
    std::string FuncName;
    std::string FilePath;
    std::string SrcCode;
    std::vector<std::string> VarNames;
    std::map<IFDSUninitializedVariables::n_t,
             std::set<IFDSUninitializedVariables::d_t>>
        IRTrace;
    [[nodiscard]] bool empty() const;
    void print(llvm::raw_ostream &OS);
  };

public:
  IFDSUninitializedVariables(const LLVMProjectIRDB *IRDB,
                             std::vector<std::string> EntryPoints = {"main"});

  ~IFDSUninitializedVariables() override = default;

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

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &Results,
                      llvm::raw_ostream &OS = llvm::outs()) override;

  [[nodiscard]] const std::map<n_t, std::set<d_t>> &getAllUndefUses() const;

  std::vector<UninitResult> aggregateResults();

private:
  std::map<n_t, std::set<d_t>> UndefValueUses;
};

} // namespace psr

#endif
