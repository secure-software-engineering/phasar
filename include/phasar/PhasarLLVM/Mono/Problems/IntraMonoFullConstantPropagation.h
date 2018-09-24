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
  typedef std::pair<const llvm::Value *, unsigned> DFF;

  IntraMonoFullConstantPropagation(LLVMBasedCFG &Cfg, const llvm::Function *F);
  virtual ~IntraMonoFullConstantPropagation() = default;

  MonoSet<DFF> join(const MonoSet<DFF> &Lhs, const MonoSet<DFF> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<DFF> &Lhs, const MonoSet<DFF> &Rhs) override;

  MonoSet<DFF> flow(const llvm::Instruction *S,
                    const MonoSet<DFF> &In) override;

  MonoMap<const llvm::Instruction *, MonoSet<DFF>> initialSeeds() override;

  std::string DtoString(std::pair<const llvm::Value *, unsigned> d) override;
};

} // namespace psr

#endif
