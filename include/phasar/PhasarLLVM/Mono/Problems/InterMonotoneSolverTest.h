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

#ifndef INTERMONOTONESOLVERTEST_H_
#define INTERMONOTONESOLVERTEST_H_

#include <algorithm>
#include <iostream>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/InterMonotoneProblem.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <string>
using namespace std;

namespace psr{

class InterMonotoneSolverTest
    : public InterMonotoneProblem<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  const llvm::Value *, LLVMBasedICFG &> {
private:
  LLVMBasedICFG &ICFG;
  vector<string> EntryPoints;

public:
  InterMonotoneSolverTest(LLVMBasedICFG &Icfg,
                          vector<string> EntryPoints = {"main"});
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
             const llvm::Instruction *RetStmt, const llvm::Instruction *RetSite,
             const MonoSet<const llvm::Value *> &In) override;

  virtual MonoSet<const llvm::Value *>
  callToRetFlow(const llvm::Instruction *CallSite,
                const llvm::Instruction *RetSite,
                const MonoSet<const llvm::Value *> &In) override;

  virtual MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
  initialSeeds() override;

  virtual string DtoString(const llvm::Value *d) override;

  virtual string CtoString(const llvm::Value *c) override;
};

}//namespace psr

#endif
