/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDBACKWARDICFG_H_

#include "phasar/PhasarLLVM/ControlFlow/ICFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardCFG.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"

#include "llvm/IR/LLVMContext.h"

namespace psr {

class LLVMBasedICFG;

class LLVMBasedBackwardICFG : public LLVMBasedBackwardCFG,
                              public ICFGBase<LLVMBasedBackwardCFG> {
  friend ICFGBase;

  class LLVMBackwardRet {
  private:
    const llvm::ReturnInst *Instance = nullptr;

  public:
    LLVMBackwardRet(llvm::LLVMContext &Ctx)
        : Instance(llvm::ReturnInst::Create(Ctx)){};
    [[nodiscard]] const llvm::ReturnInst *getInstance() const noexcept {
      return Instance;
    }
  };

  using CFGBase::print;
  using ICFGBase::print;

  using CFGBase::getAsJson;
  using ICFGBase::getAsJson;

public:
  LLVMBasedBackwardICFG(LLVMBasedICFG *ForwardICFG);

private:
  [[nodiscard]] FunctionRange getAllFunctionsImpl() const;
  [[nodiscard]] f_t getFunctionImpl(llvm::StringRef Fun) const;

  [[nodiscard]] bool isIndirectFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] bool isVirtualFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] std::vector<n_t> allNonCallStartNodesImpl() const;
  [[nodiscard]] llvm::ArrayRef<f_t>
  getCalleesOfCallAtImpl(n_t Inst) const noexcept;
  [[nodiscard]] llvm::ArrayRef<n_t> getCallersOfImpl(f_t Fun) const noexcept;
  [[nodiscard]] llvm::SmallVector<n_t> getCallsFromWithinImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2>
  getReturnSitesOfCallAtImpl(n_t Inst) const;
  void printImpl(llvm::raw_ostream &OS) const;
  [[nodiscard]] nlohmann::json getAsJsonImpl() const;

  llvm::LLVMContext BackwardRetsCtx;
  llvm::DenseMap<const llvm::Function *, LLVMBackwardRet> BackwardRets;
  llvm::DenseMap<const llvm::Instruction *, const llvm::Function *>
      BackwardRetToFunction;

  LLVMBasedICFG *ForwardICFG{};
};
} // namespace psr

#endif
