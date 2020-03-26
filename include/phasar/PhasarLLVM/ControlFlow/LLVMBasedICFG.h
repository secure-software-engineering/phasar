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
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "boost/graph/adjacency_list.hpp"

#include "phasar/PhasarLLVM/ControlFlow/ICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"
#include "phasar/Utils/SoundnessFlag.h"

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
class LLVMPointsToInfo;

class LLVMBasedICFG
    : public ICFG<const llvm::Instruction *, const llvm::Function *>,
      public virtual LLVMBasedCFG {
  friend class LLVMBasedBackwardsICFG;

private:
  ProjectIRDB &IRDB;
  CallGraphAnalysisType CGType;
  SoundnessFlag SF;
  bool UserTHInfos = true;
  bool UserPTInfos = true;
  LLVMTypeHierarchy *TH;
  LLVMPointsToInfo *PT;
  PointsToGraph WholeModulePTG;
  std::unordered_set<const llvm::Function *> VisitedFunctions;
  /// Keeps track of the call-sites already resolved
  // std::vector<const llvm::Instruction *> CallStack;

  // Keeps track of the type graph already constructed
  // TypeGraph_t typegraph;

  // Any types that could be initialized outside of the module
  // std::set<const llvm::StructType*> unsound_types;

  // The VertexProperties for our call-graph.
  struct VertexProperties {
    const llvm::Function *F = nullptr;
    VertexProperties() = default;
    VertexProperties(const llvm::Function *F);
    std::string getFunctionName() const;
  };

  // The EdgeProperties for our call-graph.
  struct EdgeProperties {
    const llvm::Instruction *CS = nullptr;
    size_t ID = 0;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Instruction *I);
    std::string getCallSiteAsString() const;
  };

  /// Specify the type of graph to be used.
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                                VertexProperties, EdgeProperties>
      bidigraph_t;

  // Let us have some handy typedefs.
  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<bidigraph_t>::vertex_iterator vertex_iterator;
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;
  typedef boost::graph_traits<bidigraph_t>::out_edge_iterator out_edge_iterator;
  typedef boost::graph_traits<bidigraph_t>::in_edge_iterator in_edge_iterator;

  /// The call graph.
  bidigraph_t CallGraph;

  /// Maps function names to the corresponding vertex id.
  std::unordered_map<const llvm::Function *, vertex_t> FunctionVertexMap;

  void constructionWalker(const llvm::Function *F, Resolver &Resolver);

  struct dependency_visitor;

public:
  /**
   * Why a multimap?  A given instruction might have multiple target functions.
   * For example, if the points-to analysis indicates that a pointer could
   * be for multiple different types.
   */
  typedef std::unordered_multimap<const llvm::Instruction *,
                                  const llvm::Function *>
      OutEdgesAndTargets;

  LLVMBasedICFG(ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
                const std::set<std::string> &EntryPoints = {},
                LLVMTypeHierarchy *TH = nullptr, LLVMPointsToInfo *PT = nullptr,
                SoundnessFlag SF = SoundnessFlag::SOUNDY);

  LLVMBasedICFG(const LLVMBasedICFG &);

  ~LLVMBasedICFG() override;

  std::set<const llvm::Function *> getAllFunctions() const override;

  bool isIndirectFunctionCall(const llvm::Instruction *n) const override;

  bool isVirtualFunctionCall(const llvm::Instruction *n) const override;

  const llvm::Function *getFunction(const std::string &fun) const override;

  /**
   * Essentially the same as `getCallsFromWithin`, but uses the callgraph
   * data directly.
   * \return all call sites within a given method.
   */
  std::vector<const llvm::Instruction *>
  getOutEdges(const llvm::Function *F) const;

  /**
   * For the supplied function, get all the output edge Instructions and
   * the corresponding Function.  This pulls data directly from the callgraph.
   *
   * \return the edges and the target function for each edge.
   */
  OutEdgesAndTargets getOutEdgeAndTarget(const llvm::Function *F) const;

  /**
   * Removes all edges found for the given instruction within the
   * sourceFunction. \return number of edges removed
   */
  size_t removeEdges(const llvm::Function *sourceFunction,
                     const llvm::Instruction *instruction);

  /**
   * Removes the vertex for the given function.
   * CAUTION: does not remove edges, invoking this on a function with
   * IN or OUT edges is a bad idea.
   * \return true iff the vertex was found and removed.
   */
  bool removeVertex(const llvm::Function *F);

  /**
   * \return the total number of in edges to the vertex representing this
   * Function.
   */
  size_t getCallerCount(const llvm::Function *F) const;

  /**
   * \return all callee methods for a given call that might be called.
   */
  std::set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *n) const override;

  /**
   * \return all caller statements/nodes of a given method.
   */
  std::set<const llvm::Instruction *>
  getCallersOf(const llvm::Function *m) const override;

  /**
   * \return all call sites within a given method.
   */
  std::set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *m) const override;

  std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *m) const override;

  std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *fun) const override;

  std::set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *n) const override;

  bool isCallStmt(const llvm::Instruction *stmt) const override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() const override;

  const llvm::Instruction *getLastInstructionOf(const std::string &name);

  std::vector<const llvm::Instruction *>
  getAllInstructionsOfFunction(const std::string &name);

  void mergeWith(const LLVMBasedICFG &other);

  bool isPrimitiveFunction(const std::string &name);

  using LLVMBasedCFG::print; // tell the compiler we wish to have both prints
  void print(std::ostream &OS = std::cout) const override;

  void printAsDot(std::ostream &OS = std::cout,
                  bool printEdgeLabels = true) const;

  void printInternalPTGAsDot(std::ostream &OS = std::cout) const;

  using LLVMBasedCFG::getAsJson; // tell the compiler we wish to have both
                                 // prints
  nlohmann::json getAsJson() const override;

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  const PointsToGraph &getWholeModulePTG() const;

  std::vector<const llvm::Function *> getDependencyOrderedFunctions();
};

} // namespace psr

#endif
