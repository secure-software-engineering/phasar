/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSUninitializedVariablesStructs_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSUninitializedVariablesStructs_H

#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/Utils/IndexWrapper.h"

#include <llvm/ADT/ArrayRef.h>

#include <llvm/IR/DerivedTypes.h>

#include <llvm/IR/Value.h>

#include <llvm-14/llvm/IR/Instructions.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace psr {

struct LLVMAnalysisDomainIndexed : public AnalysisDomain {
  public:
  using d_t = IndexWrapper<llvm::Value>;
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using c_t = LLVMBasedCFG;
  using i_t = LLVMBasedICFG;
  using db_t = LLVMProjectIRDB;
};

using LLVMIFDSAnalysisDomainIndexed =
    WithBinaryValueDomain<LLVMAnalysisDomainIndexed>;

class IFDSUninitializedVariablesIndexed
    : public IFDSTabulationProblem<LLVMIFDSAnalysisDomainIndexed> {
  struct UninitResult {
    UninitResult() = default;
    unsigned int Line = 0;
    std::string FuncName;
    std::string FilePath;
    std::string SrcCode;
    std::vector<std::string> VarNames;
    std::map<IFDSUninitializedVariablesIndexed::n_t,
             std::set<IFDSUninitializedVariablesIndexed::d_t>>
        IRTrace;
    [[nodiscard]] bool empty() const;
    void print(llvm::raw_ostream &OS);
  };

public:
  IFDSUninitializedVariablesIndexed(const LLVMProjectIRDB *IRDB,
                             std::vector<std::string> EntryPoints = {"main"});

  ~IFDSUninitializedVariablesIndexed() override = default;

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
