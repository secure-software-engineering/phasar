/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSSIGNANALYSIS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSSIGNANALYSIS_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h>

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

class IFDSSignAnalysis
    : public IFDSTabulationProblem<const llvm::Instruction *,
                                   const llvm::Value *, const llvm::Function *,
                                   const llvm::StructType *,
                                   const llvm::Value *, LLVMBasedICFG> {
public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *f_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef LLVMBasedICFG i_t;

  IFDSSignAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                   const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                   std::set<std::string> EntryPoints = {"main"});

  ~IFDSSignAnalysis() override = default;

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t callStmt,
                                                         f_t destFun) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t callSite,
                                                        f_t calleeFun,
                                                        n_t exitStmt,
                                                        n_t retSite) override;

  std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<f_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, f_t destFun) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() const override;

  bool isZeroValue(d_t d) const override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, f_t m) const override;
};

} // namespace psr

#endif
