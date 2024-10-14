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

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/SparseLLVMBasedCFGProvider.h"

#include <memory>

namespace psr {
class SparseLLVMBasedCFG;
struct SVFGCache;

class SparseLLVMBasedICFG
    : public LLVMBasedICFG,
      public SparseLLVMBasedCFGProvider<SparseLLVMBasedICFG> {
  friend SparseLLVMBasedCFGProvider<SparseLLVMBasedICFG>;

public:
  explicit SparseLLVMBasedICFG(LLVMProjectIRDB *IRDB,
                               CallGraphAnalysisType CGType,
                               llvm::ArrayRef<std::string> EntryPoints = {},
                               LLVMTypeHierarchy *TH = nullptr,
                               LLVMAliasInfoRef PT = nullptr,
                               Soundness S = Soundness::Soundy,
                               bool IncludeGlobals = true);

  /// Creates an ICFG with an already given call-graph
  explicit SparseLLVMBasedICFG(CallGraph<n_t, f_t> CG, LLVMProjectIRDB *IRDB);

  explicit SparseLLVMBasedICFG(LLVMProjectIRDB *IRDB,
                               const nlohmann::json &SerializedCG);

  ~SparseLLVMBasedICFG();

private:
  [[nodiscard]] const SparseLLVMBasedCFG &
  getSparseCFGImpl(const llvm::Function *Fun, const llvm::Value *Val) const;

  std::unique_ptr<SVFGCache> SparseCFGCache;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDICFG_H
