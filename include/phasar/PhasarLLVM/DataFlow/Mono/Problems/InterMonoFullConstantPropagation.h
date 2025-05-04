/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann, and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_MONO_PROBLEMS_INTERMONOFULLCONSTANTPROPAGATION_H
#define PHASAR_PHASARLLVM_DATAFLOW_MONO_PROBLEMS_INTERMONOFULLCONSTANTPROPAGATION_H

#include "phasar/DataFlow/Mono/InterMonoProblem.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/DataFlow/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

namespace llvm {
class Value;
class Instruction;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class DIBasedTypeHierarchy;

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

  InterMonoFullConstantPropagation(const LLVMProjectIRDB *IRDB,
                                   const DIBasedTypeHierarchy *TH,
                                   const LLVMBasedICFG *ICF,
                                   LLVMAliasInfoRef PT,
                                   std::vector<std::string> EntryPoints = {});

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

  void printContainer(llvm::raw_ostream &OS,
                      mono_container_t Con) const override;
};

} // namespace psr

#endif
