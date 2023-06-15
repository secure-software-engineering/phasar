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

#include "phasar/Pointer/AliasAnalysisType.h"

#include "llvm/Analysis/AliasAnalysis.h"

namespace llvm {
class Value;
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;

class LLVMBasedAliasAnalysis {

public:
  explicit LLVMBasedAliasAnalysis(
      LLVMProjectIRDB &IRDB, bool UseLazyEvaluation,
      AliasAnalysisType PATy = AliasAnalysisType::Basic);

  LLVMBasedAliasAnalysis(LLVMBasedAliasAnalysis &&) noexcept = default;
  LLVMBasedAliasAnalysis &
  operator=(LLVMBasedAliasAnalysis &&) noexcept = default;

  LLVMBasedAliasAnalysis(const LLVMBasedAliasAnalysis &) = delete;
  LLVMBasedAliasAnalysis &operator=(const LLVMBasedAliasAnalysis &) = delete;
  ~LLVMBasedAliasAnalysis();

  void print(llvm::raw_ostream &OS = llvm::outs()) const;

  [[nodiscard]] inline llvm::AAResults *getAAResults(llvm::Function *F) {
    if (!hasAliasInfo(*F)) {
      computeAliasInfo(*F);
    }
    return AAInfos.lookup(F);
  };

  void erase(llvm::Function *F) noexcept;

  void clear() noexcept;

  [[nodiscard]] inline AliasAnalysisType
  getPointerAnalysisType() const noexcept {
    return PATy;
  };

private:
  [[nodiscard]] bool hasAliasInfo(const llvm::Function &Fun) const;

  void computeAliasInfo(llvm::Function &Fun);

  // -- data members

  struct Impl;
  std::unique_ptr<Impl> PImpl;
  AliasAnalysisType PATy;
  llvm::DenseMap<const llvm::Function *, llvm::AAResults *> AAInfos;
};

} // namespace psr

#endif
