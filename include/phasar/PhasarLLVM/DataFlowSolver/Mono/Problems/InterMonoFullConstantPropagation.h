/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann, and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOFULLCONSTANTPROPAGATION_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOFULLCONSTANTPROPAGATION_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h>
#include <phasar/PhasarLLVM/Utils/LatticeDomain.h>
#include <phasar/Utils/BitVectorSet.h>

namespace llvm {
class Value;
class Instruction;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;
class LLVMBasedICFG;

class InterMonoFullConstantPropagation
    : public InterMonoProblem<
          const llvm::Instruction *,
          std::pair<const llvm::Value *, LatticeDomain<int64_t>>,
          const llvm::Function *, const llvm::StructType *, const llvm::Value *,
          LLVMBasedICFG> {
private:
  IntraMonoFullConstantPropagation IntraPropagation;

public:
  using n_t = const llvm::Instruction *;
  using plain_d_t = int64_t;
  using d_t = std::pair<const llvm::Value *, LatticeDomain<plain_d_t>>;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using i_t = LLVMBasedICFG;

  InterMonoFullConstantPropagation(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints = {});

  ~InterMonoFullConstantPropagation() override = default;

  BitVectorSet<d_t> merge(const BitVectorSet<d_t> &Lhs,
                         const BitVectorSet<d_t> &Rhs) override;

  bool sqSubSetEqual(const BitVectorSet<d_t> &Lhs,
                     const BitVectorSet<d_t> &Rhs) override;

  bool equal_to(const BitVectorSet<d_t> &Lhs,
                const BitVectorSet<d_t> &Rhs) override;

  std::unordered_map<n_t, BitVectorSet<d_t>> initialSeeds() override;

  BitVectorSet<d_t> normalFlow(n_t S, const BitVectorSet<d_t> &In) override;

  BitVectorSet<d_t> callFlow(n_t CallSite, f_t Callee,
                             const BitVectorSet<d_t> &In) override;

  BitVectorSet<d_t> returnFlow(n_t CallSite, f_t Callee, n_t ExitStmt,
                               n_t RetSite,
                               const BitVectorSet<d_t> &In) override;

  BitVectorSet<d_t> callToRetFlow(n_t CallSite, n_t RetSite,
                                  std::set<f_t> Callees,
                                  const BitVectorSet<d_t> &In) override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, f_t f) const override;
};

} // namespace psr

#endif
