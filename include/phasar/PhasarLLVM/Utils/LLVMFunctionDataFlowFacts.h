/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, bulletSpace and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMFUNCTIONDATAFLOWFACTS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMFUNCTIONDATAFLOWFACTS_H

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/DefaultValue.h"
#include "phasar/Utils/FunctionDataFlowFacts.h"
#include "phasar/Utils/MapUtils.h"

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"

#include <vector>

namespace psr::library_summary {

class LLVMFunctionDataFlowFacts;
[[nodiscard]] LLVMFunctionDataFlowFacts
readFromFDFF(const FunctionDataFlowFacts &Fdff,
             std::invocable<llvm::StringRef> auto GetFunctionByNameOrNull);

/// @brief A LLVM-specific mapping of FunctionDataFlowFacts
class LLVMFunctionDataFlowFacts {
public:
  LLVMFunctionDataFlowFacts() noexcept = default;
  using ParameterMappingTy = FunctionDataFlowFacts::ParameterMappingTy;

  [[nodiscard]] bool contains(const llvm::Function *Fn) const {
    return LLVMFdff.count(Fn);
  }

  [[nodiscard]] const std::vector<DataFlowFact> &
  getFacts(const llvm::Function *Fun, uint32_t Index) const {
    if (const auto *Iter = getOrDefault(LLVMFdff, Fun)) {
      return getOrDefault(*Iter, Index);
    }
    return default_value();
  }
  [[nodiscard]] const std::vector<DataFlowFact> &
  getFacts(const llvm::Function *Fun, const llvm::Argument *Arg) const {
    return getFacts(Fun, Arg->getArgNo());
  }

  [[nodiscard]] const ParameterMappingTy &
  getFactsForFunction(const llvm::Function *Fun) const {
    if (const auto *Iter = getOrDefault(LLVMFdff, Fun)) {
      return *Iter;
    }
    return default_value();
  }

  [[nodiscard]] const ParameterMappingTy *
  getFactsForFunctionOrNull(const llvm::Function *Fun) const {
    return getOrDefault(LLVMFdff, Fun);
  }

  [[nodiscard]] friend LLVMFunctionDataFlowFacts
  readFromFDFF(const FunctionDataFlowFacts &Fdff,
               std::invocable<llvm::StringRef> auto GetFunctionByNameOrNull) {
    LLVMFunctionDataFlowFacts Ret{};
    Ret.LLVMFdff.reserve(Fdff.size());

    for (const auto &Elem : Fdff) {
      if (const auto *Fun =
              std::invoke(GetFunctionByNameOrNull, Elem.getKey())) {
        Ret.LLVMFdff.try_emplace(Fun, &Elem.getValue());
      }
    }

    return Ret;
  }

private:
  llvm::DenseMap<const llvm::Function *, const ParameterMappingTy *> LLVMFdff;
};
} // namespace psr::library_summary

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMFUNCTIONDATAFLOWFACTS_H
