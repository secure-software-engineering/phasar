/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDCFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDCFG_H_

#include "phasar/PhasarLLVM/ControlFlow/CFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class LLVMBasedBackwardCFG;

template <> struct CFGTraits<LLVMBasedBackwardCFG> : CFGTraits<LLVMBasedCFG> {};

class LLVMBasedBackwardCFG : public CFGBase<LLVMBasedBackwardCFG> {
  friend CFGBase;

public:
  LLVMBasedBackwardCFG(bool IgnoreDbgInstructions = true) noexcept;
  LLVMBasedBackwardCFG(const ProjectIRDB &IRDB,
                       bool IgnoreDbgInstructions = true);

private:
  [[nodiscard]] f_t getFunctionOfImpl(n_t Inst) const noexcept;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getPredsOfImpl(n_t Inst) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2> getSuccsOfImpl(n_t Inst) const;
  [[nodiscard]] std::vector<std::pair<n_t, n_t>>
  getAllControlFlowEdgesImpl(f_t Fun) const;
  [[nodiscard]] auto getAllInstructionsOfImpl(f_t Fun) const {
    return ForwardCFG.getAllInstructionsOf(Fun);
  }
  [[nodiscard]] llvm::SmallVector<n_t, 2> getStartPointsOfImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 1> getExitPointsOfImpl(f_t Fun) const;
  [[nodiscard]] bool isCallSiteImpl(n_t Inst) const noexcept {
    return ForwardCFG.isCallSite(Inst);
  }
  [[nodiscard]] bool isExitInstImpl(n_t Inst) const noexcept;
  [[nodiscard]] bool isStartPointImpl(n_t Inst) const noexcept;
  [[nodiscard]] bool isFieldLoadImpl(n_t Inst) const noexcept {
    return ForwardCFG.isFieldLoad(Inst);
  }
  [[nodiscard]] bool isFieldStoreImpl(n_t Inst) const noexcept {
    return ForwardCFG.isFieldStore(Inst);
  }
  [[nodiscard]] bool isFallThroughSuccessorImpl(n_t Inst,
                                                n_t Succ) const noexcept;
  [[nodiscard]] bool isBranchTargetImpl(n_t Inst, n_t Succ) const noexcept;
  [[nodiscard]] bool isHeapAllocatingFunctionImpl(f_t Fun) const {
    return ForwardCFG.isHeapAllocatingFunction(Fun);
  }
  [[nodiscard]] bool isSpecialMemberFunctionImpl(f_t Fun) const {
    return ForwardCFG.isSpecialMemberFunction(Fun);
  }
  [[nodiscard]] SpecialMemberFunctionType
  getSpecialMemberFunctionTypeImpl(f_t Fun) const {
    return ForwardCFG.getSpecialMemberFunctionType(Fun);
  }
  [[nodiscard]] std::string getStatementIdImpl(n_t Inst) const {
    return ForwardCFG.getStatementId(Inst);
  }
  [[nodiscard]] llvm::StringRef getFunctionNameImpl(f_t Fun) const {
    return ForwardCFG.getFunctionName(Fun);
  }
  [[nodiscard]] std::string getDemangledFunctionNameImpl(f_t Fun) const {
    return ForwardCFG.getDemangledFunctionName(Fun);
  }
  void printImpl(f_t Fun, llvm::raw_ostream &OS) const {
    ForwardCFG.print(Fun, OS);
  }
  [[nodiscard]] nlohmann::json getAsJsonImpl(f_t Fun) const {
    return ForwardCFG.getAsJson(Fun);
  }

  LLVMBasedCFG ForwardCFG;
  llvm::DenseMap<const llvm::Function *, const llvm::Instruction *>
      BackwardRets;
  llvm::DenseMap<const llvm::Instruction *, const llvm::Function *>
      BackwardRetToFunction;
};

} // namespace psr

#endif
