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
 *  Created on: 22.08.2018
 *      Author: richard leer
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_

#include <string>
#include <vector>

#include <phasar/PhasarLLVM/Mono/InterMonotoneProblem.h>

namespace llvm {
class Instruction;
class Value;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class InterMonoTaintAnalysis
    : public InterMonotoneProblem<const llvm::Instruction *,
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
  InterMonoTaintAnalysis(ICFG_t &Icfg,
                         std::vector<std::string> EntryPoints = {"main"});
  ~InterMonoTaintAnalysis() = default;

  Domain_t join(const Domain_t &Lhs, const Domain_t &Rhs) override;

  bool sqSubSetEqual(const Domain_t &Lhs, const Domain_t &Rhs) override;

  Domain_t normalFlow(const Node_t Stmt, const Domain_t &In) override;

  Domain_t callFlow(const Node_t CallSite, const Method_t Callee,
                    const Domain_t &In) override;

  Domain_t returnFlow(const Node_t CallSite, const Method_t Callee,
                      const Node_t RetSite, const Domain_t &In) override;

  Domain_t callToRetFlow(const Node_t CallSite, const Node_t RetSite,
                         const Domain_t &In) override;

  MonoMap<Node_t, Domain_t> initialSeeds() override;

  std::string DtoString(const Domain_t d) override;

  bool recompute(const Method_t Callee) override;
};

} // namespace psr

#endif
