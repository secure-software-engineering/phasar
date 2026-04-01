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
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FunctionDataFlowFacts.h"
#include "phasar/Utils/DefaultValue.h"

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"

#include <unordered_map>
#include <vector>

namespace psr::library_summary {

class LLVMFunctionDataFlowFacts;
[[nodiscard]] LLVMFunctionDataFlowFacts
readFromFDFF(const FunctionDataFlowFacts &Fdff, const LLVMProjectIRDB &Irdb);

/// @brief A LLVM-specific mapping of FunctionDataFlowFacts
class LLVMFunctionDataFlowFacts {
public:
  LLVMFunctionDataFlowFacts() noexcept = default;
  using ParameterMappingTy = FunctionDataFlowFacts::ParameterMappingTy;

  /// insert a set of data flow facts
  void insertSet(const llvm::Function *Fun, uint32_t Index,
                 std::vector<DataFlowFact> OutSet) {

    LLVMFdff[Fun].try_emplace(Index, std::move(OutSet));
  }
  void insertSet(const llvm::Function *Fun, const llvm::Argument *Arg,
                 std::vector<DataFlowFact> OutSet) {

    insertSet(Fun, Arg->getArgNo(), std::move(OutSet));
  }

  void addElement(const llvm::Function *Fun, uint32_t Index, DataFlowFact Out) {
    LLVMFdff[Fun][Index].emplace_back(Out);
  }
  void addElement(const llvm::Function *Fun, const llvm::Argument *Arg,
                  DataFlowFact Out) {
    addElement(Fun, Arg->getArgNo(), Out);
  }

  [[nodiscard]] bool contains(const llvm::Function *Fn) {
    return LLVMFdff.count(Fn);
  }

  [[nodiscard]] const std::vector<DataFlowFact> &
  getFacts(const llvm::Function *Fun, uint32_t Index) {
    auto Iter = LLVMFdff.find(Fun);
    if (Iter != LLVMFdff.end()) {
      return Iter->second[Index];
    }
    return getDefaultValue<std::vector<DataFlowFact>>();
  }
  [[nodiscard]] const std::vector<DataFlowFact> &
  getFacts(const llvm::Function *Fun, const llvm::Argument *Arg) {
    return getFacts(Fun, Arg->getArgNo());
  }

  [[nodiscard]] const ParameterMappingTy &
  getFactsForFunction(const llvm::Function *Fun) {
    auto Iter = LLVMFdff.find(Fun);
    if (Iter != LLVMFdff.end()) {
      return Iter->second;
    }
    return getDefaultValue<ParameterMappingTy>();
  }

  friend LLVMFunctionDataFlowFacts
  readFromFDFF(const FunctionDataFlowFacts &Fdff, const LLVMProjectIRDB &Irdb);

private:
  std::unordered_map<const llvm::Function *, ParameterMappingTy> LLVMFdff;
};
} // namespace psr::library_summary

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_LLVMFUNCTIONDATAFLOWFACTS_H
