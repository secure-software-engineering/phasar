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

#include <string>
#include <vector>

#include <phasar/PhasarLLVM/Mono/InterMonoProblem.h>

namespace llvm {
class Instruction;
class Value;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class InterMonoSolverTest
    : public InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, LLVMBasedICFG &> {
protected:
  std::vector<std::string> EntryPoints;

public:
  InterMonoSolverTest(LLVMBasedICFG &Icfg,
                      std::vector<std::string> EntryPoints = {"main"});
  ~InterMonoSolverTest() override = default;

  MonoSet<const llvm::Value *>
  join(const MonoSet<const llvm::Value *> &Lhs,
       const MonoSet<const llvm::Value *> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<const llvm::Value *> &Lhs,
                     const MonoSet<const llvm::Value *> &Rhs) override;

  MonoSet<const llvm::Value *>
  normalFlow(const llvm::Instruction *Stmt,
             const MonoSet<const llvm::Value *> &In) override;

  MonoSet<const llvm::Value *>
  callFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
           const MonoSet<const llvm::Value *> &In) override;

  MonoSet<const llvm::Value *>
  returnFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
             const llvm::Instruction *ExitStmt,
             const llvm::Instruction *RetSite,
             const MonoSet<const llvm::Value *> &In) override;

  MonoSet<const llvm::Value *>
  callToRetFlow(const llvm::Instruction *CallSite,
                const llvm::Instruction *RetSite,
                MonoSet<const llvm::Function *> Callees,
                const MonoSet<const llvm::Value *> &In) override;

  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void printDataFlowFact(std::ostream &os, const llvm::Value *d) const override;

  void printMethod(std::ostream &os, const llvm::Function *m) const override;
};

} // namespace psr

#endif
