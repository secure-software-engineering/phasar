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
class DIBasedTypeHierarchy;
struct SVFGCache;

/// \brief A class that implements a sparse interprocedural control flow graph.
/// Conforms to the ICFGBase CRTP interface.
///
/// Use this in the IDESolver or IFDSSolver to profit from the SparseIFDS or
/// SparseIDE optimization after Karakaya et al. "Symbol-Specific Sparsification
/// of Interprocedural Distributive Environment Problems"
/// <https://doi.org/10.48550/arXiv.2401.14813>
class SparseLLVMBasedICFG
    : public LLVMBasedICFG,
      public SparseLLVMBasedCFGProvider<SparseLLVMBasedICFG> {
  friend SparseLLVMBasedCFGProvider<SparseLLVMBasedICFG>;

public:
  /// Constructor that delegates all arguments to the ctor of LLVMBasedICFG
  explicit SparseLLVMBasedICFG(LLVMProjectIRDB *IRDB,
                               CallGraphAnalysisType CGType,
                               llvm::ArrayRef<std::string> EntryPoints = {},
                               DIBasedTypeHierarchy *TH = nullptr,
                               LLVMAliasInfoRef PT = nullptr,
                               Soundness S = Soundness::Soundy,
                               bool IncludeGlobals = true);

  /// Creates an ICFG with an already given call-graph
  explicit SparseLLVMBasedICFG(CallGraph<n_t, f_t> CG, LLVMProjectIRDB *IRDB,
                               LLVMAliasInfoRef PT);

  explicit SparseLLVMBasedICFG(LLVMProjectIRDB *IRDB,
                               const nlohmann::json &SerializedCG,
                               LLVMAliasInfoRef PT);

  ~SparseLLVMBasedICFG();

private:
  [[nodiscard]] const SparseLLVMBasedCFG &
  getSparseCFGImpl(const llvm::Function *Fun, const llvm::Value *Val) const;

  std::unique_ptr<SVFGCache> SparseCFGCache;
  LLVMAliasInfoRef AliasAnalysis;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_CONTROLFLOW_SPARSELLVMBASEDICFG_H
