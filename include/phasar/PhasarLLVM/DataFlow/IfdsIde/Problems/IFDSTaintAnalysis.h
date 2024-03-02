/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSTAINTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSTAINTANALYSIS_H

#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include <map>
#include <memory>
#include <set>
#include <string>

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Function;
class CallBase;
} // namespace llvm

namespace psr {
class LLVMTaintConfig;

/**
 * This analysis tracks data-flows through a program. Data flows from
 * dedicated source functions, which generate tainted values, into
 * dedicated sink functions. A leak is reported once a tainted value
 * reached a sink function.
 *
 * @see TaintConfiguration on how to specify your own
 * taint-sensitive source and sink functions.
 */
class IFDSTaintAnalysis
    : public IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> {

public:
  // Setup the configuration type
  using ConfigurationTy = LLVMTaintConfig;

  /// Holds all leaks found during the analysis
  std::map<n_t, std::set<d_t>> Leaks;

  /**
   *
   * @param icfg
   * @param TSF
   * @param EntryPoints
   */
  IFDSTaintAnalysis(const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
                    const LLVMTaintConfig *Config,
                    std::vector<std::string> EntryPoints = {"main"},
                    bool TaintMainArgs = true);

  ~IFDSTaintAnalysis() override = default;

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

  bool isZeroValue(d_t FlowFact) const noexcept override;

  void emitTextReport(const SolverResults<n_t, d_t, BinaryDomain> &SR,
                      llvm::raw_ostream &OS = llvm::outs()) override;

private:
  const LLVMTaintConfig *Config{};
  LLVMAliasInfoRef PT{};
  bool TaintMainArgs{};

  bool isSourceCall(const llvm::CallBase *CB,
                    const llvm::Function *Callee) const;
  bool isSinkCall(const llvm::CallBase *CB, const llvm::Function *Callee) const;
  bool isSanitizerCall(const llvm::CallBase *CB,
                       const llvm::Function *Callee) const;

  void populateWithMayAliases(container_type &Facts,
                              const llvm::Instruction *Context) const;
  void populateWithMustAliases(container_type &Facts,
                               const llvm::Instruction *Context) const;
};
} // namespace psr

#endif
