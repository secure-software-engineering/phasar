/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDICFG_H
#define PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDICFG_H

#include "phasar/ControlFlow/CallGraph.h"
#include "phasar/ControlFlow/ICFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFGProvider.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"

#include <memory>

namespace psr {
class LLVMProjectIRDB;
class LLVMBasedICFG;
class SparseLLVMBasedCFG;
class SparseLLVMBasedICFGView;
struct SVFGCache;

template <>
struct CFGTraits<SparseLLVMBasedICFGView> : CFGTraits<LLVMBasedCFG> {};

/// Similar to SparseLLVMBasedICFG; the only difference is that this one *is* no
/// LLVMBasedICFG -- it contains a pointer to an already existing one.
/// It still owns the sparse value-flow graphs
class SparseLLVMBasedICFGView
    : public LLVMBasedCFG,
      public ICFGBase<SparseLLVMBasedICFGView>,
      public SparseLLVMBasedCFGProvider<SparseLLVMBasedICFGView> {
  friend ICFGBase;
  friend SparseLLVMBasedCFGProvider<SparseLLVMBasedICFGView>;

public:
  explicit SparseLLVMBasedICFGView(const LLVMBasedICFG *ICF);

  ~SparseLLVMBasedICFGView();

  // To make the IDESolver happy...
  operator const LLVMBasedICFG &() const noexcept { return *ICF; }

private:
  [[nodiscard]] FunctionRange getAllFunctionsImpl() const;
  [[nodiscard]] f_t getFunctionImpl(llvm::StringRef Fun) const;

  [[nodiscard]] bool isIndirectFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] bool isVirtualFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] std::vector<n_t> allNonCallStartNodesImpl() const;
  [[nodiscard]] llvm::SmallVector<n_t> getCallsFromWithinImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2>
  getReturnSitesOfCallAtImpl(n_t Inst) const;
  void printImpl(llvm::raw_ostream &OS) const;
  [[nodiscard]] nlohmann::json getAsJsonImpl() const;
  [[nodiscard]] const CallGraph<n_t, f_t> &getCallGraphImpl() const noexcept;

  [[nodiscard]] const SparseLLVMBasedCFG &
  getSparseCFGImpl(const llvm::Function *Fun, const llvm::Value *Val) const;

  const LLVMProjectIRDB *IRDB{};
  const LLVMBasedICFG *ICF{};
  std::unique_ptr<SVFGCache> SparseCFGCache;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDICFG_H
