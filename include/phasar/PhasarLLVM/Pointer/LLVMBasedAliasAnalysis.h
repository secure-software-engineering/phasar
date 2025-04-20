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
class AAResults;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;

class LLVMBasedAliasAnalysis {

public:
  /// @brief A class that can perform an analysis, which generates alias
  /// informatioan from a given intermediate representation data base.
  /// @param IRDB The project intermediate representation data base, which the
  /// analysis will generate the alias information from.
  /// @param UseLazyEvaluation Set to true to use a faster but less precise
  /// algorithm.
  /// @param PATy Boolean to determine the needed analysis type.
  explicit LLVMBasedAliasAnalysis(
      LLVMProjectIRDB &IRDB, bool UseLazyEvaluation,
      AliasAnalysisType PATy = AliasAnalysisType::Basic);

  LLVMBasedAliasAnalysis(LLVMBasedAliasAnalysis &&) noexcept = default;
  LLVMBasedAliasAnalysis &
  operator=(LLVMBasedAliasAnalysis &&) noexcept = default;

  LLVMBasedAliasAnalysis(const LLVMBasedAliasAnalysis &) = delete;
  LLVMBasedAliasAnalysis &operator=(const LLVMBasedAliasAnalysis &) = delete;
  ~LLVMBasedAliasAnalysis();

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
