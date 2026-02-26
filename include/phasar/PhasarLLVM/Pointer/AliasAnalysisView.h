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
#include <type_traits>

namespace llvm {
class Value;
class DataLayout;
class Function;
} // namespace llvm

namespace psr {
class LLVMProjectIRDB;

class FunctionAliasView {
public:
  template <typename T>
  using AliasCallbackTy = AliasResult (*)(T *, const llvm::Value *,
                                          const llvm::Value *,
                                          const llvm::DataLayout &);

  [[nodiscard]] AliasResult alias(const llvm::Value *V, const llvm::Value *Rep,
                                  const llvm::DataLayout &DL) {
    return Alias(Context, V, Rep, DL);
  }

  template <typename T, typename AliasFn>
    requires(std::is_empty_v<AliasFn> &&
             std::is_default_constructible_v<AliasFn>)
  constexpr FunctionAliasView(T *Context, AliasFn /*Alias*/) noexcept
      : Context(Context), Alias(&callAlias<T, AliasFn>) {}

private:
  template <typename T, typename AliasFn>
  static AliasResult callAlias(void *Context, const llvm::Value *V1,
                               const llvm::Value *V2,
                               const llvm::DataLayout &DL) {
    return AliasFn{}(static_cast<T *>(Context), V1, V2, DL);
  }

  void *Context{};
  AliasCallbackTy<void> Alias{};
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
