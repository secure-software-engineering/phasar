/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMZeroValue.h
 *
 *  Created on: 23.05.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMZEROVALUE_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMZEROVALUE_H

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/GlobalVariable.h"

#include <memory>

namespace llvm {
class Value;
} // namespace llvm

namespace psr {

/**
 * This class may be used to represent the special zero value for IFDS
 * and IDE problems. The LLVMZeroValue is implemented as a singleton.
 */
class LLVMZeroValue : public llvm::GlobalVariable {
private:
  LLVMZeroValue(llvm::Module &Mod);

  static constexpr llvm::StringLiteral LLVMZeroValueInternalName = "zero_value";

public:
  LLVMZeroValue(const LLVMZeroValue &Z) = delete;
  LLVMZeroValue &operator=(const LLVMZeroValue &Z) = delete;
  LLVMZeroValue(LLVMZeroValue &&Z) = delete;
  LLVMZeroValue &operator=(LLVMZeroValue &&Z) = delete;
  ~LLVMZeroValue() = default;

  [[nodiscard]] llvm::StringRef getName() const noexcept {
    return LLVMZeroValueInternalName;
  }

  // Do not specify a destructor (at all)!
  static const LLVMZeroValue *getInstance();

  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto isLLVMZeroValue = [](const llvm::Value *V) noexcept {
    return V == getInstance();
  };
};
} // namespace psr

#endif
