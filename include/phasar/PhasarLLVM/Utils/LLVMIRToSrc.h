/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMIRToSrc.h
 *
 *  Created on: 11.09.2018
 *      Author: rleer
 */

#ifndef PHASAR_UTILS_LLVMIRTOSRC_H_
#define PHASAR_UTILS_LLVMIRTOSRC_H_

#include <string>

#include "nlohmann/json.hpp"

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Argument;
class Instruction;
class Function;
class Value;
class GlobalVariable;
class Module;
} // namespace llvm

namespace psr {

std::string getVarNameFromIR(const llvm::Value *V);

std::string getFunctionNameFromIR(const llvm::Value *V);

std::string getFilePathFromIR(const llvm::Value *V);

std::string getDirectoryFromIR(const llvm::Value *V);

unsigned int getLineFromIR(const llvm::Value *V);

unsigned int getColumnFromIR(const llvm::Value *V);

std::string getSrcCodeFromIR(const llvm::Value *V);

std::string getModuleIDFromIR(const llvm::Value *V);

struct SourceCodeInfo {
  std::string SourceCodeLine;
  std::string SourceCodeFilename;
  std::string SourceCodeFunctionName;
  unsigned Line = 0;
  unsigned Column = 0;

  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] bool operator==(const SourceCodeInfo &Other) const noexcept;
  [[nodiscard]] inline bool
  operator!=(const SourceCodeInfo &Other) const noexcept {
    return !(*this == Other);
  }

  /// Similar to operator==, but takes different SourceCodeFileName locations
  /// into account
  [[nodiscard]] bool equivalentWith(const SourceCodeInfo &Other) const;
};

/// Used from the JSON library internally to implicitly convert between json and
/// SourceCodeInfo
void from_json(const nlohmann::json &J, SourceCodeInfo &Info);
/// Used from the JSON library internally to implicitly convert between json and
/// SourceCodeInfo
void to_json(nlohmann::json &J, const SourceCodeInfo &Info);

SourceCodeInfo getSrcCodeInfoFromIR(const llvm::Value *V);

} // namespace psr

#endif
