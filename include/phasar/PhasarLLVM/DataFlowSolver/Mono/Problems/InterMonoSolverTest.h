/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoSolverTest.h
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOSOLVERTEST_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOSOLVERTEST_H_

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
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
class LLVMBasedICFG;

class InterMonoSolverTest
    : public InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, const llvm::StructType *,
                              const llvm::Value *, LLVMBasedICFG,
                              BitVectorSet<const llvm::Value *>> {
public:
  using n_t = const llvm::Instruction *;
  using d_t = const llvm::Value *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using i_t = LLVMBasedICFG;
  using container_t = BitVectorSet<const llvm::Value *>;

  InterMonoSolverTest(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                      const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                      std::set<std::string> EntryPoints = {});

  ~InterMonoSolverTest() override = default;

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
};

} // namespace psr

#endif
