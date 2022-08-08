/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedICFG.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDICFG_H_
#define PHASAR_PHASARLLVM_CONTROLFLOW_LLVMBASEDICFG_H_

#include "phasar/PhasarLLVM/ControlFlow/CFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/ICFGBase.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CallGraphAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/Soundness.h"

#include "nlohmann/json.hpp"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

namespace psr {
class ProjectIRDB;
class LLVMPointsToInfo;
class ProjectIRDB;
class LLVMTypeHierarchy;

class LLVMBasedICFG;
template <> struct CFGTraits<LLVMBasedICFG> : CFGTraits<LLVMBasedCFG> {};

class LLVMBasedICFG : public LLVMBasedCFG, public ICFGBase<LLVMBasedICFG> {
  friend ICFGBase;

  struct Builder;

public:
  static constexpr llvm::StringLiteral GlobalCRuntimeModelName =
      "__psrCRuntimeGlobalCtorsModel";

  explicit LLVMBasedICFG(ProjectIRDB *IRDB, CallGraphAnalysisType CGType,
                         llvm::ArrayRef<std::string> EntryPoints = {},
                         LLVMTypeHierarchy *TH = nullptr,
                         LLVMPointsToInfo *PT = nullptr,
                         Soundness S = Soundness::Soundy,
                         bool IncludeGlobals = true);

  ~LLVMBasedICFG();

  LLVMBasedICFG(const LLVMBasedICFG &) = delete;
  LLVMBasedICFG &operator=(const LLVMBasedICFG &) = delete;

  LLVMBasedICFG(LLVMBasedICFG &&) noexcept = default;
  LLVMBasedICFG &operator=(LLVMBasedICFG &&) noexcept = default;

  [[nodiscard]] std::string
  exportICFGAsDot(bool WithSourceCodeInfo = true) const;
  [[nodiscard]] nlohmann::json
  exportICFGAsJson(bool WithSourceCodeInfo = true) const;

  [[nodiscard]] const llvm::SmallVectorImpl<f_t> &
  getAllVertexFunctions() const noexcept;

  [[nodiscard]] ProjectIRDB *getIRDB() const noexcept { return IRDB; }

  using CFGBase::print;
  using ICFGBase::print;

  using CFGBase::getAsJson;
  using ICFGBase::getAsJson;

private:
  [[nodiscard]] FunctionRange getAllFunctionsImpl() const;
  [[nodiscard]] f_t getFunctionImpl(llvm::StringRef Fun) const;

  [[nodiscard]] bool isIndirectFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] bool isVirtualFunctionCallImpl(n_t Inst) const;
  [[nodiscard]] std::vector<n_t> allNonCallStartNodesImpl() const;
  [[nodiscard]] const llvm::SmallVectorImpl<f_t> &
  getCalleesOfCallAtImpl(n_t Inst) const noexcept;
  /// TODO: Return a map_iterator on the in_edge_iterator -- How to deal with
  /// not-contained funs? assert them out?
  [[nodiscard]] const llvm::SmallVectorImpl<n_t> &
  getCallersOfImpl(f_t Fun) const noexcept;
  [[nodiscard]] llvm::SmallVector<n_t> getCallsFromWithinImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2>
  getReturnSitesOfCallAtImpl(n_t Inst) const;
  void printImpl(llvm::raw_ostream &OS) const;
  [[nodiscard]] nlohmann::json getAsJsonImpl() const;

  [[nodiscard]] llvm::Function *buildCRuntimeGlobalCtorsDtorsModel(
      llvm::Module &M, llvm::ArrayRef<llvm::Function *> UserEntryPoints);

  // -------------------- Utilities --------------------

  size_t addFunctionVertex(const llvm::Function *F);
  size_t addInstructionVertex(const llvm::Instruction *Inst);

  void addCallEdge(const llvm::Instruction *CS, const llvm::Function *Callee);
  void addCallEdge(const llvm::Instruction *CS, size_t CSId,
                   const llvm::Function *Callee);

  /// The call graph.

  /// Maps Instructions/Functions to integer IDs. Instructions and Functions
  /// have separate ID spaces
  llvm::DenseMap<const llvm::Value *, size_t> ValueIdMap;
  llvm::SmallVector<const llvm::Function *, 0> VertexFunctions;

  /// Use deque as we are giving references into these containers to the
  /// outside. Implementators may retain those references while mutating the
  /// call-graph on-the-fly which requires reference stability
  std::deque<llvm::SmallVector<const llvm::Instruction *>> CallersOf;
  std::deque<llvm::SmallVector<const llvm::Function *>> CalleesAt;

  ProjectIRDB *IRDB = nullptr;
  MaybeUniquePtr<LLVMTypeHierarchy, true> TH;
};
} // namespace psr

#endif
