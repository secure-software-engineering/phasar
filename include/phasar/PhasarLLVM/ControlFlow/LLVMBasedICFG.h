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

#include "boost/graph/adjacency_list.hpp"

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

  // The VertexProperties for our call-graph.
  struct VertexProperties {
    const llvm::Function *F = nullptr;
    VertexProperties() = default;
    VertexProperties(const llvm::Function *F) noexcept;
    [[nodiscard]] llvm::StringRef getFunctionName() const noexcept;
  };

  // The EdgeProperties for our call-graph.
  struct EdgeProperties {
    const llvm::Instruction *CS = nullptr;
    size_t ID = 0;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Instruction *I) noexcept;
    [[nodiscard]] std::string getCallSiteAsString() const;
  };

  /// Specify the type of graph to be used.
  using bidigraph_t =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                            VertexProperties, EdgeProperties>;

  // Let us have some handy typedefs.
  using vertex_t = boost::graph_traits<bidigraph_t>::vertex_descriptor;
  using vertex_iterator = boost::graph_traits<bidigraph_t>::vertex_iterator;
  using edge_t = boost::graph_traits<bidigraph_t>::edge_descriptor;
  using out_edge_iterator = boost::graph_traits<bidigraph_t>::out_edge_iterator;
  using in_edge_iterator = boost::graph_traits<bidigraph_t>::in_edge_iterator;

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

  [[nodiscard]] std::vector<f_t> getAllVertexFunctions() const;

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
  [[nodiscard]] llvm::SmallVector<f_t> getCalleesOfCallAtImpl(n_t Inst) const;
  /// TODO: Return a map_iterator on the in_edge_iterator -- How to deal with
  /// not-contained funs? assert them out?
  [[nodiscard]] llvm::SmallVector<n_t> getCallersOfImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t> getCallsFromWithinImpl(f_t Fun) const;
  [[nodiscard]] llvm::SmallVector<n_t, 2>
  getReturnSitesOfCallAtImpl(n_t Inst) const;
  void printImpl(llvm::raw_ostream &OS) const;
  [[nodiscard]] nlohmann::json getAsJsonImpl() const;

  [[nodiscard]] llvm::Function *buildCRuntimeGlobalCtorsDtorsModel(
      llvm::Module &M, llvm::ArrayRef<llvm::Function *> UserEntryPoints);

  /// The call graph.
  bidigraph_t CallGraph;
  llvm::DenseMap<const llvm::Function *, vertex_t> FunctionVertexMap;
  ProjectIRDB *IRDB = nullptr;
  MaybeUniquePtr<LLVMTypeHierarchy, true> TH;
};
} // namespace psr

#endif
