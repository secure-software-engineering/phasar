/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IntraMonoSolverTest.h
 *
 *  Created on: 06.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOSOLVERTEST_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOSOLVERTEST_H_

#include <string>
#include <set>

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h>

namespace llvm {
class Value;
class Instruction;
class StructType;
class Function;
} // namespace llvm

namespace psr {

class LLVMBasedCFG;
class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

class IntraMonoSolverTest
    : public IntraMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, const llvm::StructType *, const llvm::Value *, LLVMBasedCFG> {
public:
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *m_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef LLVMBasedCFG i_t;

  IntraMonoSolverTest(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                      const LLVMBasedCFG *CF, const LLVMPointsToInfo *PT,
                      std::set<std::string> EntryPoints = {});
  ~IntraMonoSolverTest() override = default;

  MonoSet<const llvm::Value *>
  join(const MonoSet<const llvm::Value *> &Lhs,
       const MonoSet<const llvm::Value *> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<const llvm::Value *> &Lhs,
                     const MonoSet<const llvm::Value *> &Rhs) override;

  MonoSet<const llvm::Value *>
  normalFlow(const llvm::Instruction *S,
             const MonoSet<const llvm::Value *> &In) override;

  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void printDataFlowFact(std::ostream &os, const llvm::Value *d) const override;

  void printMethod(std::ostream &os, const llvm::Function *m) const override;
};

} // namespace psr

#endif
