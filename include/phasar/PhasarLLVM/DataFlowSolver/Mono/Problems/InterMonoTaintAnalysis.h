/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoTaintAnalysis.h
 *
 *  Created on: 22.08.2018
 *      Author: richard leer
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_

#include <map>
#include <set>
#include <string>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/PhasarLLVM/Utils/TaintConfiguration.h"
#include "phasar/Utils/BitVectorSet.h"

namespace llvm {
class Instruction;
class Value;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMPointsToInfo;
class LLVMTypeHierarchy;

class InterMonoTaintAnalysis
    : public InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, const llvm::StructType *,
                              const llvm::Value *, LLVMBasedICFG,
                              BitVectorSet<const llvm::Value *>> {
private:
  const TaintConfiguration<const llvm::Value *> &TSF;
  std::map<const llvm::Instruction *, std::set<const llvm::Value *>> Leaks;

public:
  using n_t = const llvm::Instruction *;
  using d_t = const llvm::Value *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using i_t = LLVMBasedICFG;
  using container_t = BitVectorSet<const llvm::Value *>;

  using ConfigurationTy = TaintConfiguration<const llvm::Value *>;

  InterMonoTaintAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                         const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                         const TaintConfiguration<const llvm::Value *> &TSF,
                         std::set<std::string> EntryPoints = {});

  ~InterMonoTaintAnalysis() override = default;

  container_t merge(const container_t &Lhs, const container_t &Rhs) override;

  bool equal_to(const container_t &Lhs, const container_t &Rhs) override;

  container_t normalFlow(const llvm::Instruction *Stmt,
                         const container_t &In) override;

  container_t callFlow(const llvm::Instruction *CallSite,
                       const llvm::Function *Callee,
                       const container_t &In) override;

  container_t returnFlow(const llvm::Instruction *CallSite,
                         const llvm::Function *Callee,
                         const llvm::Instruction *ExitStmt,
                         const llvm::Instruction *RetSite,
                         const container_t &In) override;

  container_t callToRetFlow(const llvm::Instruction *CallSite,
                            const llvm::Instruction *RetSite,
                            std::set<const llvm::Function *> Callees,
                            const container_t &In) override;

  std::unordered_map<const llvm::Instruction *, container_t>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void printDataFlowFact(std::ostream &os, const llvm::Value *d) const override;

  void printFunction(std::ostream &os, const llvm::Function *m) const override;

  const std::map<const llvm::Instruction *, std::set<const llvm::Value *>> &
  getAllLeaks() const;
};

} // namespace psr

#endif
