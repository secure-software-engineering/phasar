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

/// A class that represents a sparse interprocedural control flow graph.
class SparseLLVMBasedICFG
    : public LLVMBasedICFG,
      public SparseLLVMBasedCFGProvider<SparseLLVMBasedICFG> {
  friend SparseLLVMBasedCFGProvider<SparseLLVMBasedICFG>;

public:
  /// @param[in, out] IRDB Intermediate representation data base. The IRDB will
  /// be changed, only if IncludeGlobals is set to true.
  /// @param[in] CGType The type of the call graph analysis.
  /// @param[in] EntryPoints The entry points of the program the IRDB is based
  /// on. Often this is just { "main" }.
  /// @param TH Type Hierarchy of the given IRDB. Type Hierarchy can only be
  /// null, if the call graph type does not need a type hierarchy. In any other
  /// case, this must not be null. An example of this is the OTF analysis.
  /// @param PT Points-to information that represents aliases.
  /// @param S Level of soundness.
  /// @param IncludeGlobals Flag to determine if globals should be included.
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
