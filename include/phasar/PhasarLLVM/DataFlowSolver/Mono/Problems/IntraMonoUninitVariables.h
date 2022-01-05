/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTRAMONOUNINITVARIABLES_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTRAMONOUNINITVARIABLES_H

#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

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

struct IntraMonoUninitVariablesDomain : LLVMAnalysisDomainDefault {
  using mono_container_t = std::set<LLVMAnalysisDomainDefault::d_t>;
};

class IntraMonoUninitVariables
    : public IntraMonoProblem<IntraMonoUninitVariablesDomain> {
public:
  using n_t = IntraMonoUninitVariablesDomain::n_t;
  using d_t = IntraMonoUninitVariablesDomain::d_t;
  using f_t = IntraMonoUninitVariablesDomain::f_t;
  using t_t = IntraMonoUninitVariablesDomain::t_t;
  using v_t = IntraMonoUninitVariablesDomain::v_t;
  using i_t = IntraMonoUninitVariablesDomain::i_t;
  using mono_container_t = IntraMonoUninitVariablesDomain::mono_container_t;

  IntraMonoUninitVariables(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedCFG *CF, const LLVMPointsToInfo *PT,
                           std::set<std::string> EntryPoints = {});

  ~IntraMonoUninitVariables() override = default;

  mono_container_t merge(const mono_container_t &Lhs,
                         const mono_container_t &Rhs) override;

  bool equal_to(const mono_container_t &Lhs,
                const mono_container_t &Rhs) override;

  mono_container_t allTop() override;

  mono_container_t normalFlow(n_t Inst, const mono_container_t &In) override;

  std::unordered_map<n_t, mono_container_t> initialSeeds() override;

  void printNode(std::ostream &OS, n_t Inst) const override;

  void printDataFlowFact(std::ostream &OS, d_t Fact) const override;

  void printFunction(std::ostream &OS, f_t Fun) const override;
};

} // namespace psr

#endif
