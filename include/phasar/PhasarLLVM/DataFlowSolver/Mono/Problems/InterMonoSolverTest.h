/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoSolverTest.h
 *
 *  Created on: 23.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTERMONOSOLVERTEST_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTERMONOSOLVERTEST_H

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/Utils/BitVectorSet.h"

namespace llvm {
class Instruction;
class Value;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMPointsToInfo;
class LLVMTypeHierarchy;

struct InterMonoSolverTestDomain : LLVMAnalysisDomainDefault {
  using mono_container_t = BitVectorSet<LLVMAnalysisDomainDefault::d_t>;
};

class InterMonoSolverTest : public InterMonoProblem<InterMonoSolverTestDomain> {
public:
  using n_t = InterMonoSolverTestDomain::n_t;
  using d_t = InterMonoSolverTestDomain::d_t;
  using f_t = InterMonoSolverTestDomain::f_t;
  using t_t = InterMonoSolverTestDomain::t_t;
  using v_t = InterMonoSolverTestDomain::v_t;
  using i_t = InterMonoSolverTestDomain::i_t;
  using mono_container_t = InterMonoSolverTestDomain::mono_container_t;

  InterMonoSolverTest(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                      const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                      std::set<std::string> EntryPoints = {});

  ~InterMonoSolverTest() override = default;

  mono_container_t merge(const mono_container_t &Lhs,
                         const mono_container_t &Rhs) override;

  bool equal_to(const mono_container_t &Lhs,
                const mono_container_t &Rhs) override;

  mono_container_t normalFlow(n_t Inst, const mono_container_t &In) override;

  mono_container_t callFlow(n_t CallSite, f_t Callee,
                            const mono_container_t &In) override;

  mono_container_t returnFlow(n_t CallSite, f_t Callee, n_t ExitStmt,
                              n_t RetSite, const mono_container_t &In) override;

  mono_container_t callToRetFlow(n_t CallSite, n_t RetSite,
                                 std::set<f_t> Callees,
                                 const mono_container_t &In) override;

  std::unordered_map<n_t, mono_container_t> initialSeeds() override;

  void printNode(llvm::raw_ostream &OS, n_t Inst) const override;

  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;

  void printFunction(llvm::raw_ostream &OS, f_t Fun) const override;
};

} // namespace psr

#endif
