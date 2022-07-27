/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

/*
 * IntraMonoFullConstantPropagation.h
 *
 *  Created on: 21.07.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTRAMONOFULLCONSTANTPROPAGATION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_MONO_PROBLEMS_INTRAMONOFULLCONSTANTPROPAGATION_H

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"
#include "phasar/Utils/BitVectorSet.h"

namespace llvm {
class Value;
class Instruction;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMBasedCFG;
class LLVMBasedICFG;
class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;
class InterMonoFullConstantPropagation;

struct IntraMonoFullConstantPropagationAnalysisDomain
    : public LLVMAnalysisDomainDefault {
  using plain_d_t = int64_t;
  using d_t = std::pair<const llvm::Value *, LatticeDomain<plain_d_t>>;
  using mono_container_t =
      std::map<const llvm::Value *, LatticeDomain<plain_d_t>>;
};

class IntraMonoFullConstantPropagation
    : public IntraMonoProblem<IntraMonoFullConstantPropagationAnalysisDomain> {
public:
  using plain_d_t =
      typename IntraMonoFullConstantPropagationAnalysisDomain::plain_d_t;
  using n_t = typename IntraMonoFullConstantPropagationAnalysisDomain::n_t;
  using d_t = typename IntraMonoFullConstantPropagationAnalysisDomain::d_t;
  using f_t = typename IntraMonoFullConstantPropagationAnalysisDomain::f_t;
  using t_t = typename IntraMonoFullConstantPropagationAnalysisDomain::t_t;
  using v_t = typename IntraMonoFullConstantPropagationAnalysisDomain::v_t;
  using i_t = typename IntraMonoFullConstantPropagationAnalysisDomain::i_t;
  using c_t = typename IntraMonoFullConstantPropagationAnalysisDomain::c_t;
  using mono_container_t =
      typename IntraMonoFullConstantPropagationAnalysisDomain::mono_container_t;

  friend class InterMonoFullConstantPropagation;

private:
  static LatticeDomain<plain_d_t>
  executeBinOperation(unsigned Op, plain_d_t Lop, plain_d_t Rop);

public:
  IntraMonoFullConstantPropagation(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedCFG *CF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints = {});

  ~IntraMonoFullConstantPropagation() override = default;

  mono_container_t normalFlow(n_t Inst, const mono_container_t &In) override;

  mono_container_t merge(const mono_container_t &Lhs,
                         const mono_container_t &Rhs) override;

  bool equal_to(const mono_container_t &Lhs,
                const mono_container_t &Rhs) override;

  std::unordered_map<n_t, mono_container_t> initialSeeds() override;

  void printNode(llvm::raw_ostream &OS, n_t Inst) const override;

  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;

  void printFunction(llvm::raw_ostream &OS, f_t Fun) const override;
};

} // namespace psr

namespace std {

template <>
struct hash<std::pair<
    const llvm::Value *,
    psr::LatticeDomain<psr::IntraMonoFullConstantPropagation::plain_d_t>>> {
  size_t operator()(const std::pair<const llvm::Value *,
                                    psr::LatticeDomain<int64_t>> &P) const {
    std::hash<const llvm::Value *> HashPtr;
    size_t HP = HashPtr(P.first);
    size_t HU = 0;
    // returns nullptr if P.second is Top or Bottom, a valid pointer otherwise
    if (const auto *Ptr =
            std::get_if<psr::IntraMonoFullConstantPropagation::plain_d_t>(
                &P.second)) {
      HU = *Ptr;
    }
    return HP ^ (HU << 1);
  }
};

} // namespace std

#endif
