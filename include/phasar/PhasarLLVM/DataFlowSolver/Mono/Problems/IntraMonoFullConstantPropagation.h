/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
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

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/IntraMonoProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/InterMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
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

struct IntraMonoFullConstantPropagationAnalysisDomain
    : public LLVMAnalysisDomainDefault {
  using d_t = std::pair<const llvm::Value *, unsigned>;
  using i_t = LLVMBasedCFG;
};

class IntraMonoFullConstantPropagation
    : public IntraMonoProblem<IntraMonoFullConstantPropagationAnalysisDomain> {
public:
  IntraMonoFullConstantPropagation(const ProjectIRDB *IRDB,
                                   const LLVMTypeHierarchy *TH,
                                   const LLVMBasedCFG *CF,
                                   const LLVMPointsToInfo *PT,
                                   std::set<std::string> EntryPoints = {});
  ~IntraMonoFullConstantPropagation() override = default;

  BitVectorSet<std::pair<const llvm::Value *, unsigned>>
  join(const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
       const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Rhs)
      override;

  bool sqSubSetEqual(
      const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Lhs,
      const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &Rhs)
      override;

  BitVectorSet<std::pair<const llvm::Value *, unsigned>>
  normalFlow(const llvm::Instruction *S,
             const BitVectorSet<std::pair<const llvm::Value *, unsigned>> &In)
      override;

  std::unordered_map<const llvm::Instruction *,
                     BitVectorSet<std::pair<const llvm::Value *, unsigned>>>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void
  printDataFlowFact(std::ostream &os,
                    std::pair<const llvm::Value *, unsigned> d) const override;

  void printFunction(std::ostream &os, const llvm::Function *m) const override;
};

} // namespace psr

#endif
