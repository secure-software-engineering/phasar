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

#include <set>
#include <string>
#include <unordered_map>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/Utils/BitVectorSet.h"

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
                              const llvm::Function *, const llvm::StructType *,
                              const llvm::Value *, LLVMBasedCFG,
                              BitVectorSet<const llvm::Value *>> {
public:
  using n_t = const llvm::Instruction *;
  using d_t = const llvm::Value *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using i_t = LLVMBasedCFG;
  using container_t = BitVectorSet<const llvm::Value *>;

  IntraMonoSolverTest(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                      const LLVMBasedCFG *CF, const LLVMPointsToInfo *PT,
                      std::set<std::string> EntryPoints = {});

  ~IntraMonoSolverTest() override = default;

  BitVectorSet<const llvm::Value *>
  merge(const BitVectorSet<const llvm::Value *> &Lhs,
        const BitVectorSet<const llvm::Value *> &Rhs) override;

  bool equal_to(const BitVectorSet<const llvm::Value *> &Lhs,
                const BitVectorSet<const llvm::Value *> &Rhs) override;

  BitVectorSet<const llvm::Value *>
  normalFlow(const llvm::Instruction *Stmt,
             const BitVectorSet<const llvm::Value *> &In) override;

  std::unordered_map<const llvm::Instruction *,
                     BitVectorSet<const llvm::Value *>>
  initialSeeds() override;

  void printNode(std::ostream &OS,
                 const llvm::Instruction *Stmt) const override;

  void printDataFlowFact(std::ostream &OS,
                         const llvm::Value *FlowFact) const override;

  void printFunction(std::ostream &OS,
                     const llvm::Function *Fun) const override;
};

} // namespace psr

#endif
