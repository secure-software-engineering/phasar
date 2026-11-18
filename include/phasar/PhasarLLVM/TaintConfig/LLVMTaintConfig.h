/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TAINTCONFIG_LLVMTAINTCONFIG_H
#define PHASAR_PHASARLLVM_TAINTCONFIG_LLVMTAINTCONFIG_H

#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include "llvm/ADT/DenseSet.h"

namespace llvm {
class Instruction;
class Value;
class Function;
} // namespace llvm

namespace psr {
class LLVMTaintConfig;
class LLVMProjectIRDB;
struct TaintConfigData;

template <> struct TaintConfigTraits<LLVMTaintConfig> {
  using n_t = const llvm::Instruction *;
  using v_t = const llvm::Value *;
  using f_t = const llvm::Function *;
};

class LLVMTaintConfig : public TaintConfigBase<LLVMTaintConfig> {
  friend TaintConfigBase;

public:
  explicit LLVMTaintConfig(const psr::LLVMProjectIRDB &Code,
                           const psr::TaintConfigData &Config);
  explicit LLVMTaintConfig(const psr::LLVMProjectIRDB &AnnotatedCode);
  explicit LLVMTaintConfig(
      TaintDescriptionCallBackTy SourceCB, TaintDescriptionCallBackTy SinkCB,
      TaintDescriptionCallBackTy SanitizerCB = {}) noexcept;

  void addSourceValue(const llvm::Value *V);
  void addSinkValue(const llvm::Value *V);
  void addSanitizerValue(const llvm::Value *V);
  void addTaintCategory(const llvm::Value *Val, llvm::StringRef AnnotationStr);
  void addTaintCategory(const llvm::Value *Val, TaintCategory Annotation);

  bool isSourceFunction(const llvm::Function *Func) const;
  bool isSinkFunction(const llvm::Function *Func) const;
  bool isSanitizerFunction(const llvm::Function *Func) const;

private:
  [[nodiscard]] bool isSourceImpl(const llvm::Value *V) const;
  [[nodiscard]] bool isSinkImpl(const llvm::Value *V) const;
  [[nodiscard]] bool isSanitizerImpl(const llvm::Value *V) const;

  /// \brief Calls Handler for all operands of Inst (maybe including Inst
  /// itself) that are generated unconditionally as tainted.
  ///
  /// If Inst is a function-call, the Callee function should be specified
  /// explicitly.
  void forAllGeneratedValuesAtImpl(
      const llvm::Instruction *Inst, const llvm::Function *Callee,
      llvm::function_ref<void(const llvm::Value *)> Handler) const;

  /// \brief Calls Handler for all operands of Inst that may generate a leak
  /// when they are tainted.
  ///
  /// If Inst is a function-call, the Callee function should be specified
  /// explicitly.
  void forAllLeakCandidatesAtImpl(
      const llvm::Instruction *Inst, const llvm::Function *Callee,
      llvm::function_ref<void(const llvm::Value *)> Handler) const;

  /// \brief Calls Handler for all operands of Inst that become sanitized after
  /// the instruction is completed.
  ///
  /// If Inst is a function-call, the Callee function should be specified
  /// explicitly.
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
  makeInitialSeedsImpl(SeedConfig Conf) const;

  void printImpl(llvm::raw_ostream &OS) const;

  // --- utilities

  void addAllFunctions(const LLVMProjectIRDB &IRDB,
                       const TaintConfigData &Config);

  void addAllVariables(const LLVMProjectIRDB &IRDB,
                       const TaintConfigData &Config);

  // --- data members
  llvm::DenseSet<const llvm::Function *> AnnotatedSourceFunctions;
  llvm::DenseSet<const llvm::Function *> AnnotatedSinkFunctions;
  llvm::DenseSet<const llvm::Function *> AnnotatedSanitizerFunctions;
  llvm::DenseSet<const llvm::Value *> SourceValues;
  llvm::DenseSet<const llvm::Value *> SinkValues;
  llvm::DenseSet<const llvm::Value *> SanitizerValues;
};

extern template class TaintConfigBase<LLVMTaintConfig>;

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_LLVMTAINTCONFIG_H
