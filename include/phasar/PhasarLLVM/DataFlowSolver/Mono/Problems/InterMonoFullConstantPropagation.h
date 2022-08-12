/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann, and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTERMONOFULLCONSTANTPROPAGATION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTERMONOFULLCONSTANTPROPAGATION_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"

namespace llvm {
class Value;
class Instruction;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

class InterMonoFullConstantPropagation
    : public IntraMonoFullConstantPropagation,
      public InterMonoProblem<IntraMonoFullConstantPropagationAnalysisDomain> {
public:
  using n_t = IntraMonoFullConstantPropagation::n_t;
  using d_t = IntraMonoFullConstantPropagation::d_t;
  using plain_d_t = IntraMonoFullConstantPropagation::plain_d_t;
  using f_t = IntraMonoFullConstantPropagation::f_t;
  using t_t = IntraMonoFullConstantPropagation::t_t;
  using v_t = IntraMonoFullConstantPropagation::v_t;
  using i_t = IntraMonoFullConstantPropagation::i_t;
  using mono_container_t = IntraMonoFullConstantPropagation::mono_container_t;

  InterMonoFullConstantPropagation(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints = {});

  ~InterMonoFullConstantPropagation() override = default;

  mono_container_t normalFlow(n_t Inst, const mono_container_t &In) override;

  mono_container_t callFlow(n_t CallSite, f_t Callee,
                            const mono_container_t &In) override;

  mono_container_t returnFlow(n_t CallSite, f_t Callee, n_t ExitStmt,
                              n_t RetSite, const mono_container_t &In) override;

  mono_container_t callToRetFlow(n_t CallSite, n_t RetSite,
                                 llvm::ArrayRef<f_t> Callees,
                                 const mono_container_t &In) override;

  mono_container_t merge(const mono_container_t &Lhs,
                         const mono_container_t &Rhs) override;

  bool equal_to(const mono_container_t &Lhs,
                const mono_container_t &Rhs) override;

  std::unordered_map<n_t, mono_container_t> initialSeeds() override;

  void printNode(llvm::raw_ostream &OS, n_t Inst) const override;

  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;

  void printFunction(llvm::raw_ostream &OS, f_t Fun) const override;

  void printContainer(llvm::raw_ostream &OS,
                      mono_container_t Con) const override;
};

} // namespace psr

#endif
