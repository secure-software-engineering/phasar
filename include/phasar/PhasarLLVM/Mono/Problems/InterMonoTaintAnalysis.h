/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoTaintAnalysis.h
 *
 *  Created on: 22.08.2018
 *      Author: richard leer
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_

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

class InterMonoTaintAnalysis
    : public InterMonoProblem<const llvm::Instruction *,
                              MonoSet<const llvm::Value *>,
                              const llvm::Function *, LLVMBasedICFG &> {
public:
  typedef const llvm::Instruction * n_t;
  typedef MonoSet<const llvm::Value *> d_t;
  typedef const llvm::Function * m_t;
  typedef LLVMBasedICFG &i_t;

protected:
  std::vector<std::string> EntryPoints;

public:
  InterMonoTaintAnalysis(i_t &Icfg,
                         std::vector<std::string> EntryPoints = {"main"});
  virtual ~InterMonoTaintAnalysis() = default;

  d_t join(const d_t &Lhs, const d_t &Rhs) override;

  bool sqSubSetEqual(const d_t &Lhs, const d_t &Rhs) override;

  d_t normalFlow(n_t Stmt, const d_t &In) override;

  d_t callFlow(n_t CallSite, m_t Callee,
                    const d_t &In) override;

  d_t returnFlow(n_t CallSite, m_t Callee, n_t RetSite,
                      const d_t &In) override;

  d_t callToRetFlow(n_t CallSite, n_t RetSite,
                         const d_t &In) override;

  MonoMap<n_t, d_t> initialSeeds() override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printMethod(std::ostream &os, m_t m) const override;

  bool recompute(m_t Callee) override;
};

} // namespace psr

#endif
