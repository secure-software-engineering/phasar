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

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

namespace llvm {
class Value;
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;

class LLVMBasedAliasAnalysis {
private:
  llvm::PassBuilder PB;

  llvm::FunctionAnalysisManager FAM;
  llvm::FunctionPassManager FPM;
  llvm::DenseMap<const llvm::Function *, llvm::AAResults *> AAInfos;

  [[nodiscard]] bool hasAliasInfo(const llvm::Function &Fun) const;

  void computeAliasInfo(llvm::Function &Fun);

public:
  LLVMBasedAliasAnalysis(LLVMProjectIRDB &IRDB, bool UseLazyEvaluation = true);

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
};

} // namespace psr

#endif
