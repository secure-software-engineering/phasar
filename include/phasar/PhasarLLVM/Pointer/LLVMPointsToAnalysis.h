/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOANALYSIS_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOANALYSIS_H_

#include <iostream>
#include <unordered_map>

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

#include "phasar/PhasarLLVM/Pointer/PointsToInfo.h"

namespace llvm {
class Value;
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class ProjectIRDB;

class LLVMPointsToAnalysis {
private:
  llvm::PassBuilder PB;
  llvm::AAManager AA;
  llvm::FunctionAnalysisManager FAM;
  llvm::FunctionPassManager FPM;
  mutable std::unordered_map<const llvm::Function *, llvm::AAResults *> AAInfos;
  PointerAnalysisType PATy;

  bool hasPointsToInfo(const llvm::Function &Fun) const;

  void computePointsToInfo(llvm::Function &Fun);

public:
  LLVMPointsToAnalysis(
      ProjectIRDB &IRDB, bool UseLazyEvaluation = true,
      PointerAnalysisType PATy = PointerAnalysisType::CFLAnders);

  ~LLVMPointsToAnalysis() = default;

  void print(std::ostream &OS = std::cout) const;

  llvm::AAResults *getAAResults(llvm::Function *F);

  PointerAnalysisType getPointerAnalysisType() const;
};

} // namespace psr

#endif
