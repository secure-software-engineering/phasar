/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonoSolverTest.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOSOLVERTEST_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOSOLVERTEST_H_

#include <string>

#include <phasar/PhasarLLVM/Mono/IntraMonoProblem.h>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedCFG;

class IntraMonoSolverTest
    : public IntraMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, LLVMBasedCFG &> {
public:
  IntraMonoSolverTest(LLVMBasedCFG &Cfg, const llvm::Function *F);
  virtual ~IntraMonoSolverTest() = default;

  MonoSet<const llvm::Value *>
  join(const MonoSet<const llvm::Value *> &Lhs,
       const MonoSet<const llvm::Value *> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<const llvm::Value *> &Lhs,
                     const MonoSet<const llvm::Value *> &Rhs) override;

  MonoSet<const llvm::Value *>
  flow(const llvm::Instruction *S,
       const MonoSet<const llvm::Value *> &In) override;

  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
  initialSeeds() override;

  std::string DtoString(const llvm::Value *d) override;
};

} // namespace psr

#endif
