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

#ifndef INTRAMONOFULLCONSTANTPROPAGATION_H_
#define INTRAMONOFULLCONSTANTPROPAGATION_H_

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h>
#include <phasar/PhasarLLVM/Mono/IntraMonotoneProblem.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <algorithm>
#include <iostream>
#include <string>
using namespace std;

class IntraMonoFullConstantPropagation
    : public IntraMonotoneProblem<const llvm::Instruction *,
                                  pair<const llvm::Value *, unsigned>,
                                  const llvm::Function *, LLVMBasedCFG &> {
 public:
  typedef pair<const llvm::Value *, unsigned> DFF;

  IntraMonoFullConstantPropagation(LLVMBasedCFG &Cfg, const llvm::Function *F);
  virtual ~IntraMonoFullConstantPropagation() = default;

  virtual MonoSet<DFF> join(const MonoSet<DFF> &Lhs,
                            const MonoSet<DFF> &Rhs) override;

  virtual bool sqSubSetEqual(const MonoSet<DFF> &Lhs,
                             const MonoSet<DFF> &Rhs) override;

  virtual MonoSet<DFF> flow(const llvm::Instruction *S,
                            const MonoSet<DFF> &In) override;

  virtual MonoMap<const llvm::Instruction *, MonoSet<DFF>> initialSeeds()
      override;

  virtual string DtoString(pair<const llvm::Value *, unsigned> d) override;
};

#endif
