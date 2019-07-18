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
  IntraMonoFullConstantPropagation(LLVMBasedCFG &Cfg, const llvm::Function *F);
  ~IntraMonoFullConstantPropagation() override = default;

  MonoSet<std::pair<const llvm::Value *, unsigned>>
  join(const MonoSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
       const MonoSet<std::pair<const llvm::Value *, unsigned>> &Rhs) override;

  bool sqSubSetEqual(
      const MonoSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
      const MonoSet<std::pair<const llvm::Value *, unsigned>> &Rhs) override;

  MonoSet<std::pair<const llvm::Value *, unsigned>> normalFlow(
      const llvm::Instruction *S,
      const MonoSet<std::pair<const llvm::Value *, unsigned>> &In) override;

  MonoMap<const llvm::Instruction *,
          MonoSet<std::pair<const llvm::Value *, unsigned>>>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void
  printDataFlowFact(std::ostream &os,
                    std::pair<const llvm::Value *, unsigned> d) const override;

  void printMethod(std::ostream &os, const llvm::Function *m) const override;
};

} // namespace psr

#endif
