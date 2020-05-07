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

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOFULLCONSTANTPROPAGATION_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTRAMONOFULLCONSTANTPROPAGATION_H_

#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"
#include "phasar/Utils/BitVectorSet.h"

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
class ProjectIRDB;

class IntraMonoFullConstantPropagation
    : public IntraMonoProblem < const llvm::Instruction *,
    std::pair<const llvm::Value *, LatticeDomain<int64_t>>,
    const llvm::Function *, const llvm::StructType *, const llvm::Value *,
    LLVMBasedCFG,
    BitVectorSet<std::pair<const llvm::Value *, LatticeDomain<int64_t>>>> {
public:
  using n_t = const llvm::Instruction *;
  using plain_d_t = int64_t;
  using d_t = std::pair<const llvm::Value *, LatticeDomain<plain_d_t>>;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;
  using i_t = LLVMBasedCFG;
  using container_t = BitVectorSet<d_t>;

  friend class InterMonoFullConstantPropagation;

private:
  static bool bitVectorHasInstr(const BitVectorSet<d_t> &set, v_t instr);

  static LatticeDomain<plain_d_t>
  executeBinOperation(const unsigned op, plain_d_t lop, plain_d_t rop);

public:
  IntraMonoFullConstantPropagation(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedCFG *CF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints = {});

  ~IntraMonoFullConstantPropagation() override = default;

  BitVectorSet<d_t> merge(const BitVectorSet<d_t> &Lhs,
                          const BitVectorSet<d_t> &Rhs) override;

  BitVectorSet<d_t> update(const BitVectorSet<d_t> &Lhs,
                           const BitVectorSet<d_t> &Rhs);

  bool equal_to(const BitVectorSet<d_t> &Lhs,
                const BitVectorSet<d_t> &Rhs) override;

  std::unordered_map<n_t, BitVectorSet<d_t>> initialSeeds() override;

  BitVectorSet<d_t> normalFlow(n_t S, const BitVectorSet<d_t> &In) override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, f_t f) const override;
};

} // namespace psr

namespace std {

template <>
struct hash<std::pair<
    const llvm::Value *,
    psr::LatticeDomain<psr::IntraMonoFullConstantPropagation::plain_d_t>>> {
  size_t operator()(const std::pair<const llvm::Value *,
                                    psr::LatticeDomain<int64_t>> &P) const {
    std::hash<const llvm::Value *> hash_ptr;
    std::hash<int64_t> hash_unsigned;
    size_t hp = hash_ptr(P.first);
    size_t hu = 0;
    // returns nullptr if P.second is Top or Bottom, a valid pointer otherwise
    if (auto Ptr =
            std::get_if<psr::IntraMonoFullConstantPropagation::plain_d_t>(
                &P.second)) {
      hu = *Ptr;
    }
    return hp ^ (hu << 1);
  }
};

} // namespace std

#endif
