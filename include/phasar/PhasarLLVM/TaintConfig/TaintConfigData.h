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
  TaintConfigData() = default;
  TaintConfigData(const llvm::Twine &Path, const LLVMProjectIRDB &IRDB);
  TaintConfigData(const nlohmann::json &Config, const LLVMProjectIRDB &IRDB);

  static TaintConfigData loadDataFromFile(const llvm::Twine &Path,
                                          const LLVMProjectIRDB &IRDB);
  void addDataToFile(const llvm::Twine &Path);

  void addAllFunctions(const LLVMProjectIRDB &IRDB,
                       const nlohmann::json &Config);
  void addAllVariables(const LLVMProjectIRDB &IRDB,
                       const nlohmann::json &Config);

  inline void addSourceValue(std::string Value) {
    SourceValues.insert(std::move(Value));
  }
  inline void addSinkValue(std::string Value) {
    SinkValues.insert(std::move(Value));
  }
  inline void addSanitizerValue(std::string Value) {
    SanitizerValues.insert(std::move(Value));
  }

  void addSourceValue(const llvm::Value *V);
  void addSinkValue(const llvm::Value *V);
  void addSanitizerValue(const llvm::Value *V);
  void addTaintCategory(const llvm::Value *Val, llvm::StringRef AnnotationStr);
  void addTaintCategory(const llvm::Value *Val, TaintCategory Annotation);

  void getValuesFromJSON(nlohmann::json JSON);

  inline const std::unordered_set<std::string> &getSourceValues() const {
    return SourceValues;
  }
  inline const std::unordered_set<std::string> &getSinkValues() const {
    return SinkValues;
  }
  inline const std::unordered_set<std::string> &getSanitizerValues() const {
    return SanitizerValues;
  }

private:
  void loadDataFromFileForThis(const llvm::Twine &Path,
                               const LLVMProjectIRDB &IRDB);
  std::unordered_set<std::string> SourceValues;
  std::unordered_set<std::string> SinkValues;
  std::unordered_set<std::string> SanitizerValues;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGDATA_H