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

#ifndef PHASAR_PHASARLLVM_DATAFLOW_MONO_PROBLEMS_INTRAMONOSOLVERTEST_H
#define PHASAR_PHASARLLVM_DATAFLOW_MONO_PROBLEMS_INTRAMONOSOLVERTEST_H

#include "phasar/DataFlow/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/BitVectorSet.h"

#include <set>
#include <string>
#include <unordered_map>

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

struct IntraMonoSolverTestAnalysisDomain : public LLVMAnalysisDomainDefault {
  using mono_container_t = BitVectorSet<LLVMAnalysisDomainDefault::d_t>;
};

class IntraMonoSolverTest
    : public IntraMonoProblem<IntraMonoSolverTestAnalysisDomain> {
public:
  using n_t = IntraMonoSolverTestAnalysisDomain::n_t;
  using d_t = IntraMonoSolverTestAnalysisDomain::d_t;
  using f_t = IntraMonoSolverTestAnalysisDomain::f_t;
  using t_t = IntraMonoSolverTestAnalysisDomain::t_t;
  using v_t = IntraMonoSolverTestAnalysisDomain::v_t;
  using i_t = IntraMonoSolverTestAnalysisDomain::i_t;
  using mono_container_t = IntraMonoSolverTestAnalysisDomain::mono_container_t;

  IntraMonoSolverTest(const LLVMProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                      const LLVMBasedCFG *CF, LLVMAliasInfoRef PT,
                      std::vector<std::string> EntryPoints = {});

  ~IntraMonoSolverTest() override = default;

  mono_container_t merge(const mono_container_t &Lhs,
                         const mono_container_t &Rhs) override;

  bool equal_to(const mono_container_t &Lhs,
                const mono_container_t &Rhs) override;

  mono_container_t normalFlow(n_t Inst, const mono_container_t &In) override;

  std::unordered_map<n_t, mono_container_t> initialSeeds() override;

  void printNode(llvm::raw_ostream &OS, n_t Inst) const override;

  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;

  void printFunction(llvm::raw_ostream &OS,
                     const llvm::Function *Fun) const override;
};

} // namespace psr

#endif
