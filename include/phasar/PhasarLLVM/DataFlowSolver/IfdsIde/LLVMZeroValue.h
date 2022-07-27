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

/**
 * This class may be used to represent the special zero value for IFDS
 * and IDE problems. The LLVMZeroValue is implemented as a singleton.
 */
class LLVMZeroValue : public llvm::GlobalVariable {
private:
  LLVMZeroValue(llvm::Module &Mod); // NOLINT(modernize-use-equals-delete)
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
    auto GetZeroMod = [] {
      static llvm::LLVMContext Ctx;
      static llvm::Module Mod("zero_module", Ctx);
      return &Mod;
    };
    static std::unique_ptr<LLVMZeroValue> ZV(new LLVMZeroValue(*GetZeroMod()));
    return ZV.get();
  }
};
} // namespace psr

#endif
