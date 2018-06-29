/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * LLVMBasedInterproceduralICFG.h
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#pragma once

#include <functional>
#include <map>
#include <string>
#include <iosfwd>
#include <vector>

#include <boost/graph/adjacency_list.hpp>

#include <phasar/PhasarLLVM/ControlFlow/ICFG.h>
#include <phasar/PhasarLLVM/Pointer/PointsToGraph.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/DB/ProjectIRDB.h>

namespace llvm {
  class Instruction;
  class Function;
  class Module;
  class Instruction;
}

namespace psr {

//Forward declaration
class CachedTypeGraph;

// Describes the strategy to be used for the instruction walker.
enum class WalkerStrategy { Simple = 0, VariableType, DeclaredType, Pointer };

extern const std::map<std::string, WalkerStrategy> StringToWalkerStrategy;

extern const std::map<WalkerStrategy, std::string> WalkerStrategyToString;

std::ostream &operator<<(std::ostream &os, const WalkerStrategy W);

// Describes the strategy that is used for resolving indirect call-sites;
enum class ResolveStrategy { CHA = 0, RTA, TA, OTF };

extern const std::map<std::string, ResolveStrategy> StringToResolveStrategy;

extern const std::map<ResolveStrategy, std::string> ResolveStrategyToString;

std::ostream &operator<<(std::ostream &os, const ResolveStrategy R);

class LLVMBasedICFG
    : public ICFG<const llvm::Instruction *, const llvm::Function *> {
private:
  WalkerStrategy W;
  ResolveStrategy R;
  const std::map<WalkerStrategy,
            std::function<void(LLVMBasedICFG *, const llvm::Function *)>>
      Walker{{WalkerStrategy::Simple,
              &LLVMBasedICFG::resolveIndirectCallWalkerSimple},
             {WalkerStrategy::VariableType,
              &LLVMBasedICFG::resolveIndirectCallWalkerSimple},
             {WalkerStrategy::DeclaredType,
              &LLVMBasedICFG::resolveIndirectCallWalkerDTA},
             {WalkerStrategy::Pointer,
              &LLVMBasedICFG::resolveIndirectCallWalkerPointerAnalysis}};
  const std::map<ResolveStrategy,
            std::function<std::set<std::string>(LLVMBasedICFG *, llvm::ImmutableCallSite)>>
      Resolver{{ResolveStrategy::CHA, &LLVMBasedICFG::resolveIndirectCallCHA},
               {ResolveStrategy::RTA, &LLVMBasedICFG::resolveIndirectCallRTA},
               {ResolveStrategy::TA, &LLVMBasedICFG::resolveIndirectCallTA},
               {ResolveStrategy::OTF, &LLVMBasedICFG::resolveIndirectCallOTF}};
  LLVMTypeHierarchy &CH;
  ProjectIRDB &IRDB;
  PointsToGraph WholeModulePTG;
  std::set<const llvm::Function *> VisitedFunctions;
  /// Keeps track of the call-sites already resolved
  std::vector<const llvm::Instruction *> CallStack;

  // Keeps track of the type graph already constructed
  std::map<const llvm::Function*, CachedTypeGraph*> tgs;

  // Any types that could be initialized outside of the module
  std::set<const llvm::StructType*> unsound_types;

  // The VertexProperties for our call-graph.
  struct VertexProperties {
    const llvm::Function *function = nullptr;
    std::string functionName;
    bool isDeclaration;
    VertexProperties() = default;
    VertexProperties(const llvm::Function *f, bool isDecl = false);
  };

  // The EdgeProperties for our call-graph.
  struct EdgeProperties {
    const llvm::Instruction *callsite = nullptr;
    std::string ir_code;
    size_t id = 0;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Instruction *i);
  };

  /// Specify the type of graph to be used.
  typedef boost::adjacency_list<boost::multisetS, boost::vecS,
                                boost::bidirectionalS, VertexProperties,
                                EdgeProperties>
      bidigraph_t;

  // Let us have some handy typedefs.
  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<bidigraph_t>::vertex_iterator vertex_iterator;
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;
  typedef boost::graph_traits<bidigraph_t>::out_edge_iterator out_edge_iterator;
  typedef boost::graph_traits<bidigraph_t>::in_edge_iterator in_edge_iterator;

  /// The call graph.
  bidigraph_t cg;

  /// Maps function names to the corresponding vertex id.
  std::map<std::string, vertex_t> function_vertex_map;

  /**
   * Resolved an indirect call using points-to information in order to
   * obtain highly precise results. Using this function might be quite expensive
   * in terms of computation time.
   *
   * @brief Resolves an indirect call using points-to information.
   * @param Call-site to be resolved.
   * @return Set of function names that might be called at this call-site.
   */
  std::set<std::string> resolveIndirectCallOTF(llvm::ImmutableCallSite CS);

  /**
   * Resolved an indirect call using class hierarchy information.
   * Using this function is absolutly cheap, but precsion almost always
   * suffers a lot.
   *
   * @brief Resolves an indirect call using call hierarchy information.
   * @param Call-site to be resolved.
   * @return Set of function names that might be called at this call-site.
   */
  std::set<std::string> resolveIndirectCallCHA(llvm::ImmutableCallSite CS);

  /**
   * Resolved an indirect call using class hierarchy information but taking
   * only types into account that are actually instantiated in the program.
   *
   * @brief Resolves an indirect call using more precise call hierarchy
   * information.
   * @param Call-site to be resolved.
   * @return Set of function names that might be called at this call-site.
   */
  std::set<std::string> resolveIndirectCallRTA(llvm::ImmutableCallSite CS);

  /**
   * Resolved an indirect call using type information that are obtained by
   * a variable type analysis or declared type analysis.
   *
   * @brief Resolves an indirect call using type information.
   * @param Call-site to be resolved.
   * @return Set of function names that might be called at this call-site.
   */
  std::set<std::string> resolveIndirectCallTA(llvm::ImmutableCallSite CS);

  /**
   * A simple function walking along the control flow resolving indirect
   * call-sites using the resolving function R. This walker does not perform
   * any analysis by itself.
   *
   * @brief A simple function walking along the control flow graph.
   * @param F function to start in
   */
  void resolveIndirectCallWalkerSimple(const llvm::Function *F);

  /**
   * A simple function walking along the control flow resolving indirect
   * call-sites using Declare Type Analysis function. This walker does not perform
   * any analysis by itself.
   *
   * @brief A function walking along the control flow graph construction a DTA Graph.
   * @param F function to start in
   */
  void resolveIndirectCallWalkerDTA(const llvm::Function *F);

  // /**
  //  * Walking along the control flow resolving indirect
  //  * call-sites using the resolving function R. This walker does perform
  //  * a variable- or declared type analysis in order to build an type
  //  * propagation graph. This type propagation graph can then be used
  //  * by the resolving function R.
  //  *
  //  * @brief Walking along the control flow graph performing a type analysis.
  //  * @param F function to start in
  //  * @param R resolving function to use for an indirect call site
  //  * @param useVTA use VTA otherwise DTA is used for type propagation graph
  //  * construction
  //  */
  // void resolveIndirectCallWalkerTypeAnalysis(
  //     const llvm::Function *F,
  //     function<std::set<std::string>(llvm::ImmutableCallSite CS)> R, bool useVTA = true);

  /**
   * Walking along the control flow resolving indirect call-sites using
   * the resolving function R. This walker does perform a highly precise
   * inter-procedural points-to graph, which is intended to be used by the
   * indirect call resolving function R.
   *
   * @brief Walks along the control flow graph performing a precise pointer
   * analysis.
   * @param F function to start in
   * @param R resolving function to use for an indirect call site
   */
  void resolveIndirectCallWalkerPointerAnalysis(const llvm::Function *F);

  struct dependency_visitor;

public:
  LLVMBasedICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB);

  LLVMBasedICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB, WalkerStrategy W,
                ResolveStrategy R,
                const std::vector<std::string> &EntryPoints = {"main"});

  LLVMBasedICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,
                const llvm::Module &M, WalkerStrategy W, ResolveStrategy R,
                std::vector<std::string> EntryPoints = {});

  virtual ~LLVMBasedICFG() noexcept;

  bool isVirtualFunctionCall(llvm::ImmutableCallSite CS);

  const llvm::Function *getMethodOf(const llvm::Instruction *stmt) override;

  std::vector<const llvm::Instruction *>
  getPredsOf(const llvm::Instruction *I) override;

  std::vector<const llvm::Instruction *>
  getSuccsOf(const llvm::Instruction *I) override;

  std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
  getAllControlFlowEdges(const llvm::Function *fun) override;

  std::vector<const llvm::Instruction *>
  getAllInstructionsOf(const llvm::Function *fun) override;

  bool isExitStmt(const llvm::Instruction *stmt) override;

  bool isStartPoint(const llvm::Instruction *stmt) override;

  bool isFallThroughSuccessor(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) override;

  bool isBranchTarget(const llvm::Instruction *stmt,
                      const llvm::Instruction *succ) override;

  std::string getMethodName(const llvm::Function *fun) override;
  std::string getStatementId(const llvm::Instruction *stmt) override;

  const llvm::Function *getMethod(const std::string &fun) override;

  std::set<const llvm::Function *>
  getCalleesOfCallAt(const llvm::Instruction *n) override;

  std::set<const llvm::Instruction *> getCallersOf(const llvm::Function *m) override;

  std::set<const llvm::Instruction *>
  getCallsFromWithin(const llvm::Function *m) override;

  std::set<const llvm::Instruction *>
  getStartPointsOf(const llvm::Function *m) override;

  std::set<const llvm::Instruction *>
  getExitPointsOf(const llvm::Function *fun) override;

  std::set<const llvm::Instruction *>
  getReturnSitesOfCallAt(const llvm::Instruction *n) override;

  bool isCallStmt(const llvm::Instruction *stmt) override;

  std::set<const llvm::Instruction *> allNonCallStartNodes() override;

  const llvm::Instruction *getLastInstructionOf(const std::string &name);

  std::vector<const llvm::Instruction *>
  getAllInstructionsOfFunction(const std::string &name);

  void mergeWith(const LLVMBasedICFG &other);

  bool isPrimitiveFunction(const std::string &name);

  void print();

  void printAsDot(const std::string &filename);

  void printInternalPTGAsDot(const std::string &filename);

  json getAsJson() override;

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  void exportPATBCJSON();

  PointsToGraph &getWholeModulePTG();

  std::vector<std::string> getDependencyOrderedFunctions();
};

} // namespace psr
