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
                              MonoSet<const llvm::Value *>,
                              const llvm::Function *, LLVMBasedICFG &> {
public:
  using Node_t = const llvm::Instruction *;
  using Domain_t = MonoSet<const llvm::Value *>;
  using Method_t = const llvm::Function *;
  using ICFG_t = LLVMBasedICFG &;

protected:
  std::vector<std::string> EntryPoints;

public:
  InterMonoSolverTest(ICFG_t &Icfg,
                      std::vector<std::string> EntryPoints = {"main"});
  virtual ~InterMonoSolverTest() = default;

  Domain_t join(const Domain_t &Lhs, const Domain_t &Rhs) override;

  bool sqSubSetEqual(const Domain_t &Lhs, const Domain_t &Rhs) override;

  Domain_t normalFlow(Node_t Stmt, const Domain_t &In) override;

  Domain_t callFlow(Node_t CallSite, Method_t Callee,
                    const Domain_t &In) override;

  Domain_t returnFlow(Node_t CallSite, Method_t Callee, Node_t RetSite,
                      const Domain_t &In) override;

  Domain_t callToRetFlow(Node_t CallSite, Node_t RetSite,
                         const Domain_t &In) override;

  MonoMap<Node_t, Domain_t> initialSeeds() override;

  void printNode(std::ostream &os, Node_t n) const override;

  void printDataFlowFact(std::ostream &os, Domain_t d) const override;

  void printMethod(std::ostream &os, Method_t m) const override;

  bool recompute(Method_t Callee) override;
};

} // namespace psr

#endif
