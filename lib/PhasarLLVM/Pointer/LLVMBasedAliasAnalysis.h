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

#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Utils/Fn.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Passes/PassBuilder.h"

namespace llvm {
class Value;
class Function;
class Instruction;
class AAResults;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;

class LLVMBasedAliasAnalysis : public AliasAnalysisView {
public:
  explicit LLVMBasedAliasAnalysis(
      LLVMProjectIRDB &IRDB, bool UseLazyEvaluation,
      AliasAnalysisType PATy = AliasAnalysisType::Basic);

  ~LLVMBasedAliasAnalysis() override;

private:
  FunctionAliasView doGetAAResults(const llvm::Function *F) override {
    if (!hasAliasInfo(*F)) {
      // NOLINTNEXTLINE - FIXME when it is fixed in LLVM
      computeAliasInfo(const_cast<llvm::Function &>(*F));
    }
    return createFAView(AAInfos.lookup(F));
  };

  void doErase(llvm::Function *F) noexcept override;

  void doClear() noexcept override;

  static AliasResult aliasImpl(llvm::AAResults *, const llvm::Value *,
                               const llvm::Value *, const llvm::DataLayout &);
  [[nodiscard]] constexpr FunctionAliasView
  createFAView(llvm::AAResults *AAR) noexcept {
    return {AAR, fn<aliasImpl>};
  }

  [[nodiscard]] bool hasAliasInfo(const llvm::Function &Fun) const;

  void computeAliasInfo(llvm::Function &Fun);

  // -- data members
  llvm::PassBuilder PB;
  llvm::FunctionAnalysisManager FAM;
  llvm::FunctionPassManager FPM;
  llvm::DenseMap<const llvm::Function *, llvm::AAResults *> AAInfos;
};

} // namespace psr

#endif
