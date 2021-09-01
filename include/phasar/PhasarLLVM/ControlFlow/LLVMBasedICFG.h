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

#include <iosfwd>
#include <iostream>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "boost/container/flat_set.hpp"
#include "boost/graph/adjacency_list.hpp"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/Utils/Soundness.h"

namespace llvm {
class Instruction;
class Function;
class Module;
class Instruction;
class BitCastInst;
} // namespace llvm

namespace psr {

class Resolver;
class ProjectIRDB;
class LLVMTypeHierarchy;

class LLVMBasedICFG
    : public ICFG<const llvm::Instruction *, const llvm::Function *>,
      public virtual LLVMBasedCFG {
  friend class LLVMBasedBackwardsICFG;

  using GlobalCtorTy = std::multimap<size_t, llvm::Function *>;
  using GlobalDtorTy = std::multimap<size_t, llvm::Function *, std::greater<>>;

private:
  ProjectIRDB &IRDB;
  CallGraphAnalysisType CGType;
  Soundness S;
  bool UserTHInfos = true;
  bool UserPTInfos = true;
  LLVMTypeHierarchy *TH;
  LLVMPointsToInfo *PT;
  std::unique_ptr<Resolver> Res;
  std::unordered_set<const llvm::Function *> VisitedFunctions;
  llvm::SmallPtrSet<llvm::Function *, 2> UserEntryPoints;

  GlobalCtorTy GlobalCtors;
  GlobalDtorTy GlobalDtors;

  // llvm::SmallDenseMap<F, typename GlobalCtorTy::const_iterator, 2>
  // GlobalCtorFn; llvm::SmallDenseMap<F, typename GlobalDtorTy::const_iterator,
  // 2> GlobalDtorFn;

  llvm::Function *GlobalCleanupFn = nullptr;

  llvm::SmallDenseMap<const llvm::Module *, llvm::Function *>
      GlobalRegisteredDtorsCaller;
  /// Keeps track of the call-sites already resolved
  // std::vector<const llvm::Instruction *> CallStack;

  // Keeps track of the type graph already constructed
  // TypeGraph_t typegraph;

  // Any types that could be initialized outside of the module
  // std::set<const llvm::StructType*> unsound_types;

  // The worklist for direct callee resolution.
  std::vector<const llvm::Function *> FunctionWL;

  // Map indirect calls to the number of possible targets found for it. Fixpoint
  // is not reached when more targets are found.
  std::unordered_map<const llvm::Instruction *, unsigned> IndirectCalls;
  // The VertexProperties for our call-graph.
  struct VertexProperties {
    const llvm::Function *F = nullptr;
    VertexProperties() = default;
    VertexProperties(const llvm::Function *F);
    [[nodiscard]] std::string getFunctionName() const;
  };

  // The EdgeProperties for our call-graph.
  struct EdgeProperties {
    const llvm::Instruction *CS = nullptr;
    size_t ID = 0;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Instruction *I);
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

  /// The call graph.
  bidigraph_t CallGraph;

  /// Maps functions to the corresponding vertex id.
  std::unordered_map<const llvm::Function *, vertex_t> FunctionVertexMap;

  void processFunction(const llvm::Function *F, Resolver &Resolver,
                       bool &FixpointReached);

  bool constructDynamicCall(const llvm::Instruction *I, Resolver &Resolver);

  std::unique_ptr<Resolver> makeResolver(ProjectIRDB &IRDB,
                                         CallGraphAnalysisType CGT,
                                         LLVMTypeHierarchy &TH,
                                         LLVMPointsToInfo &PT);

  template <typename MapTy>
  static void insertGlobalCtorsDtorsImpl(MapTy &Into, const llvm::Module *M,
                                         llvm::StringRef Fun) {
    const auto *Gtors = M->getGlobalVariable(Fun);
    if (Gtors == nullptr) {
      return;
    }

    if (const auto *FunArray = llvm::dyn_cast<llvm::ArrayType>(
            Gtors->getType()->getPointerElementType())) {
      if (const auto *ConstFunArray =
              llvm::dyn_cast<llvm::ConstantArray>(Gtors->getInitializer())) {
        for (const auto &Op : ConstFunArray->operands()) {
          if (const auto *FunDesc = llvm::dyn_cast<llvm::ConstantStruct>(Op)) {
            auto *Fun = llvm::dyn_cast<llvm::Function>(FunDesc->getOperand(1));
            const auto *Prio =
                llvm::dyn_cast<llvm::ConstantInt>(FunDesc->getOperand(0));
            if (Fun && Prio) {
              auto PrioInt = size_t(Prio->getLimitedValue(SIZE_MAX));
              Into.emplace(PrioInt, Fun);
            }
          }
        }
      }
    }
  }

  llvm::Function *buildCRuntimeGlobalDtorsModel(llvm::Module &M);
  const llvm::Function *buildCRuntimeGlobalCtorsDtorsModel(llvm::Module &M);

  struct dependency_visitor;

public:
  static constexpr llvm::StringLiteral GlobalCRuntimeModelName =
      "__psrCRuntimeGlobalCtorsModel";

  /**
   * Why a multimap?  A given instruction might have multiple target functions.
   * For example, if the points-to analysis indicates that a pointer could
   * be for multiple different types.
   */
  using OutEdgesAndTargets = std::unordered_multimap<const llvm::Instruction *,
                                                     const llvm::Function *>;

  LLVMBasedICFG(ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                const std::set<std::string> &EntryPoints = {},
                LLVMTypeHierarchy *TH = nullptr, LLVMPointsToInfo *PT = nullptr,
                Soundness S = Soundness::Soundy, bool IncludeGlobals = false);

  LLVMBasedICFG(const LLVMBasedICFG &);

  ~LLVMBasedICFG() override;

  // Re-override getSuccsOf and getPredsOf from LLVMBasedCFG to incorporate
  // information about global ctors and globale dtors
  [[nodiscard]] std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *Inst) const override;

  [[nodiscard]] std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *Inst) const override;

  [[nodiscard]] const llvm::Function *getFirstGlobalCtorOrNull() const;

  [[nodiscard]] const llvm::Function *getLastGlobalDtorOrNull() const;

  /**
   * \return all of the functions in the IRDB, this may include some not in the
   * callgraph
   */
  [[nodiscard]] std::set<const llvm::Function *>
  getAllFunctions() const override;

  /**
   * A boost flat_set is used here because we already have the functions in
   * order, so building it is fast since we can always add to the end.  We get
   * the performance and space benefits of array-backed storage and all the
   * functionality of a set.
   *
   * \return all of the functions which are represented by a vertex in the
   * callgraph.
   */
  [[nodiscard]] boost::container::flat_set<const llvm::Function *>
  getAllVertexFunctions() const;

  bool isIndirectFunctionCall(const llvm::Instruction *N) const override;

  bool isVirtualFunctionCall(const llvm::Instruction *N) const override;

  [[nodiscard]] const llvm::Function *
  getFunction(const std::string &Fun) const override;

  /**
   * Essentially the same as `getCallsFromWithin`, but uses the callgraph
   * data directly.
   * \return all call sites within a given method.
   */
  std::vector<const llvm::Instruction *>
  getOutEdges(const llvm::Function *Fun) const;

  /**
   * For the supplied function, get all the output edge Instructions and
   * the corresponding Function.  This pulls data directly from the callgraph.
   *
   * \return the edges and the target function for each edge.
   */
  OutEdgesAndTargets getOutEdgeAndTarget(const llvm::Function *Fun) const;

  /**
   * Removes all edges found for the given instruction within the
   * sourceFunction. \return number of edges removed
   */
  size_t removeEdges(const llvm::Function *F, const llvm::Instruction *Inst);

  /**
   * Removes the vertex for the given function.
   * CAUTION: does not remove edges, invoking this on a function with
   * IN or OUT edges is a bad idea.
   * \return true iff the vertex was found and removed.
   */
  bool removeVertex(const llvm::Function *Fun);

  /**
   * \return the total number of in edges to the vertex representing this
   * Function.
   */
  size_t getCallerCount(const llvm::Function *Fun) const;

  /**
   * \return all callee methods for a given call that might be called.
   */
  [[nodiscard]] std::set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *N) const override;

  void forEachCalleeOfCallAt(
      const llvm::Instruction *I,
      llvm::function_ref<void(const llvm::Function *)> Callback) const;

  /**
   * \return all caller statements/nodes of a given method.
   */
  [[nodiscard]] std::set<const llvm::Instruction *>
  getCallersOf(const llvm::Function *Fun) const override;

  /**
   * \return all call sites within a given method.
   */
  [[nodiscard]] std::set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *Fun) const override;

  [[nodiscard]] std::set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *N) const override;

  [[nodiscard]] std::set<const llvm::Instruction *>
  allNonCallStartNodes() const override;

  void mergeWith(const LLVMBasedICFG &Other);

  [[nodiscard]] CallGraphAnalysisType getCallGraphAnalysisType() const;

  using LLVMBasedCFG::print; // tell the compiler we wish to have both prints
  void print(std::ostream &OS = std::cout) const override;

  void printAsDot(std::ostream &OS = std::cout,
                  bool printEdgeLabels = true) const;

  void printInternalPTGAsDot(std::ostream &OS = std::cout) const;

  using LLVMBasedCFG::getAsJson; // tell the compiler we wish to have both
                                 // prints
  [[nodiscard]] nlohmann::json getAsJson() const override;

  void printAsJson(std::ostream &OS = std::cout) const;

  /// Create an IR based JSON export of the whole ICFG.
  ///
  /// Note: The exported JSON contains a list of all edges in this ICFG
  [[nodiscard]] nlohmann::json exportICFGAsJson() const;

  /// Create a JSON export of the whole ICFG similar to exportICFGAsJson()
  /// enriched with source-code information on every edge and ignoring debug
  /// instructions
  [[nodiscard]] nlohmann::json exportICFGAsSourceCodeJson() const;

  [[nodiscard]] unsigned getNumOfVertices() const;

  [[nodiscard]] unsigned getNumOfEdges() const;

  std::vector<const llvm::Function *> getDependencyOrderedFunctions();

  [[nodiscard]] const llvm::Function *
  getRegisteredDtorsCallerOrNull(const llvm::Module *Mod);

  template <typename Fn> void forEachGlobalCtor(Fn &&fn) const {
    for (auto [Prio, Fun] : GlobalCtors) {
      fn(static_cast<const llvm::Function *>(Fun));
    }
  }

  template <typename Fn> void forEachGlobalDtor(Fn &&fn) const {
    for (auto [Prio, Fun] : GlobalDtors) {
      fn(static_cast<const llvm::Function *>(Fun));
    }
  }

protected:
  void collectGlobalCtors() override;

  void collectGlobalDtors() override;

  void collectGlobalInitializers() override;

  void collectRegisteredDtors() override;
};

} // namespace psr

#endif
