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
#include <set>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/Config/ContainerConfiguration.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Mono/InterMonotoneProblem.h>

namespace psr {

class InterMonotoneSolverTest
    : public InterMonotoneProblem<const llvm::Instruction *,
                                  MonoSet<const llvm::Value *>, const llvm::Function *,
                                  LLVMBasedICFG> {
public:
  using Node_t      = const llvm::Instruction*;
  using Domain_t    = MonoSet<const llvm::Value*>;
  using Method_t    = const llvm::Function*;
  using ICFG_t      = LLVMBasedICFG;

protected:
  std::vector<std::string> EntryPoints;

public:
  InterMonotoneSolverTest(ICFG_t &Icfg,
                          std::vector<std::string> EntryPoints = {"main"});
  virtual ~InterMonotoneSolverTest() = default;

  virtual Domain_t
  join(const Domain_t &Lhs,
       const Domain_t &Rhs) override;

  virtual bool sqSubSetEqual(const Domain_t &Lhs,
                             const Domain_t &Rhs) override;

  virtual Domain_t
  normalFlow(const Node_t Stmt,
             const Domain_t &In) override;

  virtual Domain_t
  callFlow(const Node_t CallSite, const Method_t Callee,
           const Domain_t &In) override;

  virtual Domain_t
  returnFlow(const Node_t CallSite, const Method_t Callee,
             const Node_t RetSite,
             const Domain_t &In) override;

  virtual Domain_t
  callToRetFlow(const Node_t CallSite,
                const Node_t RetSite,
                const Domain_t &In) override;

  virtual MonoMap<Node_t, Domain_t>
  initialSeeds() override;

  virtual std::string DtoString(const Domain_t d) override;

  virtual bool recompute(const Method_t Callee) override;
};

} // namespace psr
