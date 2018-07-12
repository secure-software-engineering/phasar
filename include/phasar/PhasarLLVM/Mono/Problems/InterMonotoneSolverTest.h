/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * MonotoneSolverTest.h
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#pragma once

#include <algorithm>
#include <string>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/InterMonotoneProblem.h>

namespace psr {

class InterMonotoneSolverTest
    : public InterMonotoneProblem<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  LLVMBasedICFG> {
protected:
  std::vector<std::string> EntryPoints;

public:
  InterMonotoneSolverTest(LLVMBasedICFG &Icfg,
                          std::vector<std::string> EntryPoints = {"main"});
  virtual ~InterMonotoneSolverTest() = default;

  virtual MonoSet<const llvm::Value *>
  join(const MonoSet<const llvm::Value *> &Lhs,
       const MonoSet<const llvm::Value *> &Rhs) override;

  virtual bool sqSubSetEqual(const MonoSet<const llvm::Value *> &Lhs,
                             const MonoSet<const llvm::Value *> &Rhs) override;

  virtual MonoSet<const llvm::Value *>
  normalFlow(const llvm::Instruction *Stmt,
             const MonoSet<const llvm::Value *> &In) override;

  virtual MonoSet<const llvm::Value *>
  callFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
           const MonoSet<const llvm::Value *> &In) override;

  virtual MonoSet<const llvm::Value *>
  returnFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
             const llvm::Instruction *RetSite,
             const MonoSet<const llvm::Value *> &In) override;

  virtual MonoSet<const llvm::Value *>
  callToRetFlow(const llvm::Instruction *CallSite,
                const llvm::Instruction *RetSite,
                const MonoSet<const llvm::Value *> &In) override;

  virtual MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
  initialSeeds() override;

  virtual std::string DtoString(const llvm::Value *d) override;

  virtual bool recompute(const llvm::Function* Callee) override;
};

} // namespace psr
