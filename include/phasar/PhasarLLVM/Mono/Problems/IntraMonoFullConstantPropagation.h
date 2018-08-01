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

#include <phasar/PhasarLLVM/Mono/IntraMonotoneProblem.h>

namespace llvm {
  class Value;
  class Instruction;
  class Function;
}

namespace psr {

class LLVMBasedCFG;

class IntraMonoFullConstantPropagation
    : public IntraMonotoneProblem<const llvm::Instruction *,
                                  std::pair<const llvm::Value *, unsigned>,
                                  const llvm::Function *, LLVMBasedCFG &> {
public:
  typedef std::pair<const llvm::Value *, unsigned> DFF;

  IntraMonoFullConstantPropagation(LLVMBasedCFG &Cfg, const llvm::Function *F);
  virtual ~IntraMonoFullConstantPropagation() = default;

  virtual MonoSet<DFF> join(const MonoSet<DFF> &Lhs,
                            const MonoSet<DFF> &Rhs) override;

  virtual bool sqSubSetEqual(const MonoSet<DFF> &Lhs,
                             const MonoSet<DFF> &Rhs) override;

  virtual MonoSet<DFF> flow(const llvm::Instruction *S,
                            const MonoSet<DFF> &In) override;

  virtual MonoMap<const llvm::Instruction *, MonoSet<DFF>>
  initialSeeds() override;

  virtual std::string DtoString(std::pair<const llvm::Value *, unsigned> d) override;
};

} // namespace psr

#endif
