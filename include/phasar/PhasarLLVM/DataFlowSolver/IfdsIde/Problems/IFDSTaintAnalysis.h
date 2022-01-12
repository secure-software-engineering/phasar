/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IFDSTAINTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IFDSTAINTANALYSIS_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

// Forward declaration of types for which we only use its pointer or ref type
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
struct HasNoConfigurationType;

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
private:
  const TaintConfig &Config;

  bool isSourceCall(const llvm::CallBase *CB,
                    const llvm::Function *Callee) const;
  bool isSinkCall(const llvm::CallBase *CB, const llvm::Function *Callee) const;
  bool isSanitizerCall(const llvm::CallBase *CB,
                       const llvm::Function *Callee) const;

  void populateWithMayAliases(std::set<d_t> &Facts) const;
  void populateWithMustAliases(std::set<d_t> &Facts) const;

public:
  // Setup the configuration type
  using ConfigurationTy = TaintConfig;

  /// Holds all leaks found during the analysis
  std::map<n_t, std::set<d_t>> Leaks;

  /**
   *
   * @param icfg
   * @param TSF
   * @param EntryPoints
   */
  IFDSTaintAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                    const TaintConfig &Config,
                    std::set<std::string> EntryPoints = {"main"});

  ~IFDSTaintAnalysis() override = default;

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite) override;

  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               std::set<f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestFun) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const override;

  bool isZeroValue(d_t FlowFact) const override;

  void printNode(std::ostream &OS, n_t Inst) const override;

  void printDataFlowFact(std::ostream &OS, d_t FlowFact) const override;

  void printFunction(std::ostream &OS, f_t Fun) const override;

  void emitTextReport(const SolverResults<n_t, d_t, BinaryDomain> &SR,
                      std::ostream &OS = std::cout) override;
};
} // namespace psr

#endif
