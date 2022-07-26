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

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_LLVMZEROVALUE_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_LLVMZEROVALUE_H

#include <memory>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Alignment.h"

namespace llvm {
class Value;
} // namespace llvm

namespace psr {

// do not touch, its only purpose is to make ZeroValue working
inline const std::unique_ptr<llvm::LLVMContext>
    LLVMZeroValueCTX(new llvm::LLVMContext);
inline const std::unique_ptr<llvm::Module>
    LLVMZeroValueMod(new llvm::Module("zero_module", *LLVMZeroValueCTX));

/**
 * This class may be used to represent the special zero value for IFDS
 * and IDE problems. The LLVMZeroValue is implemented as a singleton.
 */
class LLVMZeroValue : public llvm::GlobalVariable {
private:
  LLVMZeroValue()
      : llvm::GlobalVariable(
            *LLVMZeroValueMod, llvm::Type::getIntNTy(*LLVMZeroValueCTX, 2),
            true, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
            llvm::ConstantInt::get(*LLVMZeroValueCTX,
                                   llvm::APInt(/*nbits*/ 2,
                                               /*value*/ 0,
                                               /*signed*/ true)),
            LLVMZeroValueInternalName) {
    setAlignment(llvm::MaybeAlign(4));
  }
  ~LLVMZeroValue() = default;
  static constexpr auto LLVMZeroValueInternalName = "zero_value";

public:
  LLVMZeroValue(const LLVMZeroValue &Z) = delete;
  LLVMZeroValue &operator=(const LLVMZeroValue &Z) = delete;
  LLVMZeroValue(LLVMZeroValue &&Z) = delete;
  LLVMZeroValue &operator=(LLVMZeroValue &&Z) = delete;

  [[nodiscard]] llvm::StringRef getName() const {
    return LLVMZeroValueInternalName;
  }

  static bool isLLVMZeroValue(const llvm::Value *V) {
    return V == getInstance();
  }

  // Do not specify a destructor (at all)!
  static const LLVMZeroValue *getInstance() {
    static const auto *ZV = new LLVMZeroValue;
    return ZV;
  }
};
} // namespace psr

#endif
