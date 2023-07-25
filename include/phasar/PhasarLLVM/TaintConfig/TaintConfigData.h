/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Maximilian Leo Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H
#define PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfigBase.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include <unordered_set>

#include <nlohmann/json_fwd.hpp>

namespace psr {
class TaintConfigData;

class TaintConfigData {
public:
  TaintConfigData(const LLVMProjectIRDB &IRDB, const nlohmann::json &Config);

  void addSourceValue(const llvm::Value *V);
  void addSinkValue(const llvm::Value *V);
  void addSanitizerValue(const llvm::Value *V);
  void addTaintCategory(const llvm::Value *Val, llvm::StringRef AnnotationStr);
  void addTaintCategory(const llvm::Value *Val, TaintCategory Annotation);
  // --- utilities

  void addAllFunctions(const LLVMProjectIRDB &IRDB,
                       const nlohmann::json &Config);

  inline std::unordered_set<const llvm::Value *> getAllSourceValues() const {
    return SourceValues;
  }
  inline std::unordered_set<const llvm::Value *> getAllSinkValues() const {
    return SinkValues;
  }
  inline std::unordered_set<const llvm::Value *> getAllSanitizerValues() const {
    return SanitizerValues;
  }

private:
  std::unordered_set<const llvm::Value *> SourceValues;
  std::unordered_set<const llvm::Value *> SinkValues;
  std::unordered_set<const llvm::Value *> SanitizerValues;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H
