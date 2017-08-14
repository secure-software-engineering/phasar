/*
 * MonotoneSolverTest.hh
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#ifndef INTERMONOTONESOLVERTEST_HH_
#define INTERMONOTONESOLVERTEST_HH_

#include "../../icfg/LLVMBasedICFG.hh"
#include "../../monotone/InterMonotoneProblem.hh"
#include "../../../lib/LLVMShorthands.hh"
#include <algorithm>
#include <iostream>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <string>
using namespace std;

class InterMonotoneSolverTest
    : public InterMonotoneProblem<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  LLVMBasedICFG &> {
private:
  LLVMBasedICFG &ICFG;

public:
  InterMonotoneSolverTest(LLVMBasedICFG &Icfg);
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

  virtual string D_to_string(const llvm::Value *d) override;
};

#endif
