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
                              const llvm::Value *, LLVMBasedICFG> {
private:
  const TaintConfiguration<const llvm::Value *> &TSF;
  std::map<const llvm::Instruction *, std::set<const llvm::Value *>> Leaks;

public:
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *f_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef LLVMBasedICFG i_t;

  using ConfigurationTy = TaintConfiguration<const llvm::Value *>;

  InterMonoTaintAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                         const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                         const TaintConfiguration<const llvm::Value *> &TSF,
                         std::set<std::string> EntryPoints = {});
  ~InterMonoTaintAnalysis() override = default;

  BitVectorSet<const llvm::Value *>
  join(const BitVectorSet<const llvm::Value *> &Lhs,
       const BitVectorSet<const llvm::Value *> &Rhs) override;

  bool sqSubSetEqual(const BitVectorSet<const llvm::Value *> &Lhs,
                     const BitVectorSet<const llvm::Value *> &Rhs) override;

  BitVectorSet<const llvm::Value *>
  normalFlow(const llvm::Instruction *Stmt,
             const BitVectorSet<const llvm::Value *> &In) override;

  BitVectorSet<const llvm::Value *>
  callFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
           const BitVectorSet<const llvm::Value *> &In) override;

  BitVectorSet<const llvm::Value *>
  returnFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
             const llvm::Instruction *ExitStmt,
             const llvm::Instruction *RetSite,
             const BitVectorSet<const llvm::Value *> &In) override;

  BitVectorSet<const llvm::Value *>
  callToRetFlow(const llvm::Instruction *CallSite,
                const llvm::Instruction *RetSite,
                std::set<const llvm::Function *> Callees,
                const BitVectorSet<const llvm::Value *> &In) override;

  std::unordered_map<const llvm::Instruction *,
                     BitVectorSet<const llvm::Value *>>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void printDataFlowFact(std::ostream &os, const llvm::Value *d) const override;

  void printFunction(std::ostream &os, const llvm::Function *m) const override;

  const std::map<const llvm::Instruction *, std::set<const llvm::Value *>> &
  getAllLeaks() const;
};

} // namespace psr

#endif
