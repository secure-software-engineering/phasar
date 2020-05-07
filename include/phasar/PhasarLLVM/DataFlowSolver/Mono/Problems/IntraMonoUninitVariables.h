/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOUNINITVARIABLES_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOUNINITVARIABLES_H_

#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h>
#include <phasar/Utils/BitVectorSet.h>

namespace llvm {
class Value;
class Instruction;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMTypeHierarchy;
class LLVMPointsToInfo;
class LLVMBasedCFG;
class LLVMBasedICFG;

class IntraMonoUninitVariables
    : public IntraMonoProblem < const llvm::Instruction *,
    const llvm::Value *, const llvm::Function *, const llvm::StructType *,
    const llvm::Value *, LLVMBasedCFG, BitVectorSet<const llvm::Value *>> {
public:
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *f_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef LLVMBasedCFG i_t;
  typedef BitVectorSet<d_t> container_t;

  IntraMonoUninitVariables(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedCFG *CF, const LLVMPointsToInfo *PT,
                           std::set<std::string> EntryPoints = {});
  ~IntraMonoUninitVariables() override = default;

  container_t merge(const container_t &Lhs, const container_t &Rhs) override;

  bool equal_to(const container_t &Lhs, const container_t &Rhs) override;

  container_t allTop() override;

  container_t normalFlow(const llvm::Instruction *S,
                         const container_t &In) override;

  std::unordered_map<const llvm::Instruction *, container_t>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, const llvm::Function *m) const override;
};

} // namespace psr

#endif
