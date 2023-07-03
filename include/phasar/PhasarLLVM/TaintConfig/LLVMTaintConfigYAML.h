/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TAINTCONFIG_LLVMTAINTCONFIGYAML_H
#define PHASAR_PHASARLLVM_TAINTCONFIG_LLVMTAINTCONFIGYAML_H
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include <string>
#include <unordered_set>

namespace psr {
class LLVMTaintConfigYAML;
class LLVMProjectIRDB;

template <> struct TaintConfigTraits<LLVMTaintConfigYAML> {
  using n_t = const llvm::Instruction *;
  using v_t = const llvm::Value *;
  using f_t = const llvm::Function *;
};

class LLVMTaintConfigYAML : public TaintConfigBase<LLVMTaintConfigYAML> {
  friend TaintConfigBase;

public:
  explicit LLVMTaintConfigYAML(const psr::LLVMProjectIRDB &Code,
                               const llvm::Twine &Path);
  explicit LLVMTaintConfigYAML(const psr::LLVMProjectIRDB &AnnotatedCode);
  explicit LLVMTaintConfigYAML(
      TaintDescriptionCallBackTy SourceCB, TaintDescriptionCallBackTy SinkCB,
      TaintDescriptionCallBackTy SanitizerCB = {}) noexcept;
  void loadYAML(const llvm::Twine &Path);
  void addSourceValue(const llvm::Value *V);
  void addSinkValue(const llvm::Value *V);
  void addSanitizerValue(const llvm::Value *V);
  void addTaintCategory(const llvm::Value *Val, llvm::StringRef AnnotationStr);
  void addTaintCategory(const llvm::Value *Val, TaintCategory Annotation);

private:
  [[nodiscard]] bool isSourceImpl(const llvm::Value *V) const;
  [[nodiscard]] bool isSinkImpl(const llvm::Value *V) const;
  [[nodiscard]] bool isSanitizerImpl(const llvm::Value *V) const;

  void forAllGeneratedValuesAtImpl(
      const llvm::Instruction *Inst, const llvm::Function *Callee,
      llvm::function_ref<void(const llvm::Value *)> Handler) const;

  void forAllLeakCandidatesAtImpl(
      const llvm::Instruction *Inst, const llvm::Function *Callee,
      llvm::function_ref<void(const llvm::Value *)> Handler) const;

  void forAllSanitizedValuesAtImpl(
      const llvm::Instruction *Inst, const llvm::Function *Callee,
      llvm::function_ref<void(const llvm::Value *)> Handler) const;

  [[nodiscard]] bool generatesValuesAtImpl(const llvm::Instruction *Inst,
                                           const llvm::Function *Callee) const;
  [[nodiscard]] bool mayLeakValuesAtImpl(const llvm::Instruction *Inst,
                                         const llvm::Function *Callee) const;
  [[nodiscard]] bool sanitizesValuesAtImpl(const llvm::Instruction *Inst,
                                           const llvm::Function *Callee) const;

  [[nodiscard]] TaintCategory getCategoryImpl(const llvm::Value *V) const;

  [[nodiscard]] std::map<const llvm::Instruction *,
                         std::set<const llvm::Value *>>
  makeInitialSeedsImpl() const;

  void printImpl(llvm::raw_ostream &OS) const;

  // --- utilities

  void addAllFunctions(const LLVMProjectIRDB &IRDB);
  // --- data members
  std::unordered_set<const llvm::Value *> SourceValues;
  std::unordered_set<const llvm::Value *> SinkValues;
  std::unordered_set<const llvm::Value *> SanitizerValues;
};

extern template class TaintConfigBase<LLVMTaintConfigYAML>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_LLVMTAINTCONFIGYAML_H
