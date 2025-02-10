/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_ALIASANALYSISVIEW_H
#define PHASAR_PHASARLLVM_POINTER_ALIASANALYSISVIEW_H

#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"

#include <memory>

namespace llvm {
class Value;
class DataLayout;
class Function;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;

class FunctionAliasView {
public:
  using AliasCallbackTy = AliasResult (*)(void *, const llvm::Value *,
                                          const llvm::Value *,
                                          const llvm::DataLayout &);

  [[nodiscard]] inline AliasResult alias(const llvm::Value *V,
                                         const llvm::Value *Rep,
                                         const llvm::DataLayout &DL) {
    return Alias(Context, V, Rep, DL);
  }

  constexpr FunctionAliasView(void *Context, AliasCallbackTy Alias) noexcept
      : Context(Context), Alias(Alias) {}

private:
  void *Context{};
  AliasCallbackTy Alias{};
};

class AliasAnalysisView {
public:
  constexpr AliasAnalysisView(AliasAnalysisType PATy) noexcept : PATy(PATy) {}

  virtual ~AliasAnalysisView() = default;

  [[nodiscard]] FunctionAliasView getAAResults(const llvm::Function *F) {
    assert(F != nullptr);
    return doGetAAResults(F);
  }

  void erase(llvm::Function *F) noexcept {
    assert(F != nullptr);
    doErase(F);
  }

  void clear() noexcept { doClear(); }

  [[nodiscard]] constexpr AliasAnalysisType
  getPointerAnalysisType() const noexcept {
    return PATy;
  };

  [[nodiscard]] static std::unique_ptr<AliasAnalysisView>
  create(LLVMProjectIRDB &IRDB, bool UseLazyEvaluation, AliasAnalysisType PATy);

private:
  static std::unique_ptr<AliasAnalysisView>
  createLLVMBasedAnalysis(LLVMProjectIRDB &IRDB, bool UseLazyEvaluation,
                          AliasAnalysisType PATy);

  virtual FunctionAliasView doGetAAResults(const llvm::Function *F) = 0;
  virtual void doErase(llvm::Function *F) noexcept = 0;
  virtual void doClear() noexcept = 0;

  AliasAnalysisType PATy{};
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_ALIASANALYSISVIEW_H
