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

#include "phasar/Utils/Fn.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/GlobalVariable.h"

#include <memory>

namespace llvm {
class Value;
} // namespace llvm

namespace psr {

/// \brief This class may be used to represent the special zero value (aka. Λ)
/// for IFDS and IDE problems. The LLVMZeroValue is implemented as a singleton.
///
class LLVMZeroValue : public llvm::GlobalVariable {
private:
  LLVMZeroValue(llvm::Module &Mod);

  static constexpr llvm::StringLiteral LLVMZeroValueInternalName = "zero_value";
  static bool isZeroValueImpl(const llvm::Value *V) noexcept {
    return V == getInstance();
  }

public:
  LLVMZeroValue(const LLVMZeroValue &Z) = delete;
  LLVMZeroValue &operator=(const LLVMZeroValue &Z) = delete;
  LLVMZeroValue(LLVMZeroValue &&Z) = delete;
  LLVMZeroValue &operator=(LLVMZeroValue &&Z) = delete;
  ~LLVMZeroValue() = default;

  [[nodiscard]] llvm::StringRef getName() const noexcept {
    return LLVMZeroValueInternalName;
  }

  /// Gets the singleton instance of the special zero value (aka. Λ).
  [[nodiscard]] static const LLVMZeroValue *getInstance();

  /// Checks, whether the given llvm::Value * is the special zero-value (aka.
  /// Λ).
  ///
  /// You can use this as follows:
  /// \code
  /// return strongUpdateStore(Store, LLVMZeroValue::isLLVMZeroValue);
  /// \endcode
  // NOLINTNEXTLINE(readability-identifier-naming)
  static constexpr auto isLLVMZeroValue = fn<isZeroValueImpl>;
};
} // namespace psr

#endif
