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

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
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

class InterMonoSolverTest : public InterMonoProblem<LLVMAnalysisDomainDefault> {
public:
  InterMonoSolverTest(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                      const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                      std::set<std::string> EntryPoints = {});
  ~InterMonoSolverTest() override = default;

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
};

} // namespace psr

#endif
