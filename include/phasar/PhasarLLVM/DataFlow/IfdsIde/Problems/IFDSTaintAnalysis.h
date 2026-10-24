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

#include "phasar/DataFlow/IfdsIde/IfdsIdeProblemMixin.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Utils/LLVMFunctionDataFlowFacts.h"
#include "phasar/Utils/WithAnalysisPrinterMixin.h"

#include <map>
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
 * @see LLVMTaintConfig on how to specify your own
 * taint-sensitive source and sink functions.
 */
class IFDSTaintAnalysis
    : public IfdsIdeProblemMixin<LLVMIFDSAnalysisDomainDefault>,
      public WithAnalysisPrinterMixin<LLVMIFDSAnalysisDomainDefault> {
  struct KillsAtFn {
    const IFDSTaintAnalysis *Self{};

    [[nodiscard]] std::optional<int32_t> operator()(n_t Curr,
                                                    d_t CurrNode) const;
  };

public:
  // Setup the configuration type
  using ConfigurationTy = LLVMTaintConfig;

  /// Holds all leaks found during the analysis
  std::map<n_t, std::set<d_t>> Leaks;

  IFDSTaintAnalysis(const LLVMProjectIRDB *IRDB, LLVMAliasInfoRef PT,
                    const LLVMTaintConfig *Config,
                    std::vector<std::string> EntryPoints = {"main"},
                    bool TaintMainArgs = true,
                    bool EnableStrongUpdateStore = true);

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ);

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun);

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite);

  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               llvm::ArrayRef<f_t> Callees);

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite, f_t DestFun);

  InitialSeeds<n_t, d_t, l_t> initialSeeds();

  [[nodiscard]] d_t createZeroValue() const;

  void emitTextReport(GenericSolverResults<n_t, d_t, BinaryDomain> SR,
                      llvm::raw_ostream &OS = llvm::outs());

  [[nodiscard]] bool
  isInteresting(const llvm::Instruction *Inst) const noexcept;

  [[nodiscard]] KillsAtFn killsAt() const { return {.Self = this}; }

private:
  const LLVMTaintConfig *Config{};
  LLVMAliasInfoRef PT{};
  bool TaintMainArgs{};
  bool EnableStrongUpdateStore{};
  library_summary::LLVMFunctionDataFlowFacts Llvmfdff;

  bool isSourceCall(const llvm::CallBase *CB,
                    const llvm::Function *Callee) const;
  bool isSinkCall(const llvm::CallBase *CB, const llvm::Function *Callee) const;
  bool isSanitizerCall(const llvm::CallBase *CB,
                       const llvm::Function *Callee) const;

  void populateWithMayAliases(container_type &Facts,
                              const llvm::Instruction *AliasQueryInst) const;
  void populateWithMustAliases(container_type &Facts,
                               const llvm::Instruction *AliasQueryInst) const;
};
} // namespace psr

#endif
