/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMBASEDALIASANALYSIS_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMBASEDALIASANALYSIS_H_

#include "phasar/PhasarLLVM/Pointer/AliasAnalysisType.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

namespace llvm {
class Value;
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class ProjectIRDB;

class LLVMBasedAliasAnalysis {
private:
  llvm::PassBuilder PB;
  llvm::AAManager AA;
  llvm::FunctionAnalysisManager FAM;
  llvm::FunctionPassManager FPM;
  llvm::DenseMap<const llvm::Function *, llvm::AAResults *> AAInfos;
  AliasAnalysisType PATy;

  [[nodiscard]] bool hasAliasInfo(const llvm::Function &Fun) const;

  void computeAliasInfo(llvm::Function &Fun);

public:
  LLVMBasedAliasAnalysis(ProjectIRDB &IRDB, bool UseLazyEvaluation = true,
                         AliasAnalysisType PATy = AliasAnalysisType::CFLAnders);

  ~LLVMBasedAliasAnalysis() = default;

  void print(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] inline llvm::AAResults *getAAResults(llvm::Function *F) {
    if (!hasAliasInfo(*F)) {
      computeAliasInfo(*F);
    }
    return AAInfos.lookup(F);
  };

  void erase(llvm::Function *F);

  void clear();

  [[nodiscard]] inline AliasAnalysisType getPointerAnalysisType() const {
    return PATy;
  };
};

} // namespace psr

#endif
