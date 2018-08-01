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
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOTONESOLVERTEST_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOTONESOLVERTEST_H_

#include <string>

#include <phasar/PhasarLLVM/Mono/IntraMonotoneProblem.h>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedCFG;

class IntraMonotoneSolverTest
    : public IntraMonotoneProblem<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  LLVMBasedCFG &> {
public:
  IntraMonotoneSolverTest(LLVMBasedCFG &Cfg, const llvm::Function *F);
  virtual ~IntraMonotoneSolverTest() = default;

  virtual MonoSet<const llvm::Value *>
  join(const MonoSet<const llvm::Value *> &Lhs,
       const MonoSet<const llvm::Value *> &Rhs) override;

  virtual bool sqSubSetEqual(const MonoSet<const llvm::Value *> &Lhs,
                             const MonoSet<const llvm::Value *> &Rhs) override;

  virtual MonoSet<const llvm::Value *>
  flow(const llvm::Instruction *S,
       const MonoSet<const llvm::Value *> &In) override;

  virtual MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
  initialSeeds() override;

  virtual std::string DtoString(const llvm::Value *d) override;
};

} // namespace psr

#endif
