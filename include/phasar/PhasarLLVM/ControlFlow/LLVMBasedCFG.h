/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDCFG_H_

#include "phasar/PhasarLLVM/ControlFlow/CFGBase.h"

#include "nlohmann/json.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

namespace llvm {
class Function;
} // namespace llvm

namespace psr {
class LLVMBasedCFG;
class LLVMBasedBackwardCFG;

template <> struct CFGTraits<LLVMBasedCFG> {
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
};

template <> struct CFGTraits<LLVMBasedBackwardCFG> : CFGTraits<LLVMBasedCFG> {};

namespace detail {
template <typename Derived> class LLVMBasedCFGImpl : public CFGBase<Derived> {
  friend CFGBase<Derived>;
  friend class LLVMBasedBackwardCFG;

public:
  using typename CFGBase<Derived>::n_t;
  using typename CFGBase<Derived>::f_t;

  [[nodiscard]] bool getIgnoreDbgInstructions() const noexcept {
    return IgnoreDbgInstructions;
  }

protected:
  LLVMBasedCFGImpl(bool IgnoreDbgInstructions = true) noexcept
      : IgnoreDbgInstructions(IgnoreDbgInstructions) {}

  bool IgnoreDbgInstructions = false;

  [[nodiscard]] f_t getFunctionOfImpl(n_t Inst) const noexcept;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getPredsOfImpl(n_t Inst) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getSuccsOfImpl(n_t Inst) const;
  [[nodiscard]] std::vector<std::pair<n_t, n_t>>
  getAllControlFlowEdgesImpl(f_t Fun) const;
  [[nodiscard]] auto getAllInstructionsOfImpl(f_t Fun) const {
    return llvm::map_range(llvm::instructions(Fun),
                           [](const llvm::Instruction &Inst) { return &Inst; });
  }
  [[nodiscard]] llvm::SmallVector<n_t, 2> getStartPointsOfImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getExitPointsOfImpl(f_t Fun) const;
  [[nodiscard]] bool isCallSiteImpl(n_t Inst) const noexcept {
    return llvm::isa<llvm::CallBase>(Inst);
  }
  [[nodiscard]] bool isExitInstImpl(n_t Inst) const noexcept {
    return llvm::isa<llvm::ReturnInst>(Inst);
  }
  [[nodiscard]] bool isStartPointImpl(n_t Inst) const noexcept;
  [[nodiscard]] bool isFieldLoadImpl(n_t Inst) const noexcept;
  [[nodiscard]] bool isFieldStoreImpl(n_t Inst) const noexcept;
  [[nodiscard]] bool isFallThroughSuccessorImpl(n_t Inst,
                                                n_t Succ) const noexcept;
  [[nodiscard]] bool isBranchTargetImpl(n_t Inst, n_t Succ) const noexcept;
  [[nodiscard]] bool isHeapAllocatingFunctionImpl(f_t Fun) const;
  [[nodiscard]] bool isSpecialMemberFunctionImpl(f_t Fun) const {
    return this->getSpecialMemberFunctionType(Fun) !=
           SpecialMemberFunctionType{};
  }
  [[nodiscard]] SpecialMemberFunctionType
  getSpecialMemberFunctionTypeImpl(f_t Fun) const;
  [[nodiscard]] std::string getStatementIdImpl(n_t Inst) const;
  [[nodiscard]] auto getFunctionNameImpl(f_t Fun) const {
    return Fun->getName();
  }
  [[nodiscard]] std::string getDemangledFunctionNameImpl(f_t Fun) const;
  void printImpl(f_t Fun, llvm::raw_ostream &OS) const { OS << *Fun; }
  [[nodiscard]] nlohmann::json getAsJsonImpl(f_t /*Fun*/) const { return ""; }
};
} // namespace detail

class LLVMBasedCFG : public detail::LLVMBasedCFGImpl<LLVMBasedCFG> {
  friend CFGBase;
  friend class LLVMBasedBackwardCFG;

public:
  LLVMBasedCFG(bool IgnoreDbgInstructions = true) noexcept
      : detail::LLVMBasedCFGImpl<LLVMBasedCFG>(IgnoreDbgInstructions) {}

  [[nodiscard]] nlohmann::json exportCFGAsJson(const llvm::Function *F) const;
  [[nodiscard]] nlohmann::json
  exportCFGAsSourceCodeJson(const llvm::Function *F) const;
};

extern template class detail::LLVMBasedCFGImpl<LLVMBasedCFG>;
extern template class detail::LLVMBasedCFGImpl<LLVMBasedBackwardCFG>;

} // namespace psr

#endif
