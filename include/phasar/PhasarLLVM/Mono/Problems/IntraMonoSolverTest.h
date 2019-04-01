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
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *m_t;
  typedef LLVMBasedCFG &i_t;

  IntraMonoSolverTest(i_t Cfg, m_t F);
  virtual ~IntraMonoSolverTest() = default;

  MonoSet<d_t> join(const MonoSet<d_t> &Lhs, const MonoSet<d_t> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<d_t> &Lhs, const MonoSet<d_t> &Rhs) override;

  MonoSet<d_t> normalFlow(n_t S, const MonoSet<d_t> &In) override;

  MonoMap<n_t, MonoSet<d_t>> initialSeeds() override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printMethod(std::ostream &os, m_t m) const override;
};

} // namespace psr

#endif
