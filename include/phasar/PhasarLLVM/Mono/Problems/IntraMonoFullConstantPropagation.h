/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonoFullConstantPropagation.h
 *
 *  Created on: 21.07.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOFULLCONSTANTPROPAGATION_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOFULLCONSTANTPROPAGATION_H_

#include <string>
#include <utility> // std::pair

#include <phasar/PhasarLLVM/Mono/IntraMonoProblem.h>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedCFG;

class IntraMonoFullConstantPropagation
    : public IntraMonoProblem<const llvm::Instruction *,
                              std::pair<const llvm::Value *, unsigned>,
                              const llvm::Function *, LLVMBasedCFG &> {
public:
  using Node_t = const llvm::Instruction *;
  using Domain_t = std::pair<const llvm::Value *, unsigned>;
  using Method_t = const llvm::Function *;
  using CFG_t = LLVMBasedCFG &;

  IntraMonoFullConstantPropagation(CFG_t Cfg, Method_t F);
  virtual ~IntraMonoFullConstantPropagation() = default;

  MonoSet<Domain_t> join(const MonoSet<Domain_t> &Lhs,
                         const MonoSet<Domain_t> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<Domain_t> &Lhs,
                     const MonoSet<Domain_t> &Rhs) override;

  MonoSet<Domain_t> flow(Node_t S, const MonoSet<Domain_t> &In) override;

  MonoMap<Node_t, MonoSet<Domain_t>> initialSeeds() override;

  void printNode(std::ostream &os, Node_t n) const override;

  void printDataFlowFact(std::ostream &os, Domain_t d) const override;

  void printMethod(std::ostream &os, Method_t m) const override;
};

} // namespace psr

#endif
