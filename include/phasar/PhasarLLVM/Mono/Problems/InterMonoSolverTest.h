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
    : public InterMonoProblem<const llvm::Instruction *,
                              const llvm::Value *,
                              const llvm::Function *, LLVMBasedICFG &> {
public:
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *m_t;
  typedef LLVMBasedICFG &i_t;

protected:
  std::vector<std::string> EntryPoints;

public:
  InterMonoSolverTest(i_t &Icfg,
                      std::vector<std::string> EntryPoints = {"main"});
  virtual ~InterMonoSolverTest() = default;

  MonoSet<d_t> join(const MonoSet<d_t> &Lhs, const MonoSet<d_t> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<d_t> &Lhs, const MonoSet<d_t> &Rhs) override;

  MonoSet<d_t> normalFlow(n_t Stmt, const MonoSet<d_t> &In) override;

  MonoSet<d_t> callFlow(n_t CallSite, m_t Callee, const MonoSet<d_t> &In) override;

  MonoSet<d_t> returnFlow(n_t CallSite, m_t Callee, n_t RetSite, const MonoSet<d_t> &In) override;

  MonoSet<d_t> callToRetFlow(n_t CallSite, n_t RetSite, const MonoSet<d_t> &In) override;

  MonoMap<n_t, MonoSet<d_t>> initialSeeds() override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printMethod(std::ostream &os, m_t m) const override;

};

} // namespace psr

#endif
