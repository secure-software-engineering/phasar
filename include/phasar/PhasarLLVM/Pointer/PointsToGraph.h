/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PointsToGraph.h
 *
 *  Created on: 08.02.2017
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_POINTER_POINTSTOGRAPH_H_
#define PHASAR_PHASARLLVM_POINTER_POINTSTOGRAPH_H_

#include <vector>

#include <boost/graph/adjacency_list.hpp>

#include <llvm/IR/CallSite.h>

#include <json.hpp>

#include <phasar/Config/Configuration.h>

namespace llvm {
class Value;
class Module;
class Instruction;
class AAResults;
class Function;
class Type;
} // namespace llvm

namespace psr {

using json = nlohmann::json;

// See the following llvm classes for comprehension
// http://llvm.org/docs/doxygen/html/AliasAnalysis_8cpp_source.html
// http://llvm.org/docs/doxygen/html/AliasAnalysisEvaluator_8cpp_source.html

// See also the different kinds of alias analyses
//	#include "llvm/Analysis/AliasAnalysis.h"
//	#include "llvm/Analysis/BasicAliasAnalysis.h"
//	#include "llvm/Analysis/CFG.h"
//	#include "llvm/Analysis/CFLAliasAnalysis.h"
//	#include "llvm/Analysis/CaptureTracking.h"
//	#include "llvm/Analysis/GlobalsModRef.h"
//	#include "llvm/Analysis/ObjCARCAliasAnalysis.h"
//	#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
//	#include "llvm/Analysis/ScopedNoAliasAA.h"
//	#include "llvm/Analysis/TargetLibraryInfo.h"
//	#include "llvm/Analysis/TypeBasedAliasAnalysis.h"

void PrintResults(const char *Msg, bool P, const llvm::Value *V1,
                  const llvm::Value *V2, const llvm::Module *M);

inline void PrintModRefResults(const char *Msg, bool P,
                               const llvm::Instruction *I,
                               const llvm::Value *Ptr, const llvm::Module *M);

inline void PrintModRefResults(const char *Msg, bool P,
                               const llvm::CallSite CSA,
                               const llvm::CallSite CSB, const llvm::Module *M);

inline void PrintLoadStoreResults(const char *Msg, bool P,
                                  const llvm::Value *V1, const llvm::Value *V2,
                                  const llvm::Module *M);

enum class PointerAnalysisType { CFLSteens, CFLAnders };

extern const std::map<std::string, PointerAnalysisType>
    StringToPointerAnalysisType;

extern const std::map<PointerAnalysisType, std::string>
    PointerAnalysisTypeToString;

// TODO: add a more high level description.
/**
 * 	This class is a representation of a points-to graph. It is possible to
 * 	construct a points-to graph for a single function using the results of
 *  the llvm alias analysis or merge several points-to graphs into a single
 *	points-to graph, e.g. to construct a whole program points-to graph.
 *
 *	The graph itself is undirectional and can have labeled edges.
 *
 *	@brief Represents the points-to graph of a function.
 */
class PointsToGraph {
public:
  // Call-graph firends
  friend class LLVMBasedICFG;
  /**
   * 	@brief Holds the information of a vertex in the points-to graph.
   */
  struct VertexProperties {
    /**
     * This might be an Instruction, an Operand of an Instruction, Global
     * Variable
     * or a formal Argument.
     */
    const llvm::Value *value = nullptr;

    /// Holds the llvm IR code for that vertex.
    std::string ir_code;

    /**
     *  For an instruction the id is equal to the annotated id of the
     * instruction.
     *  In all other cases it's zero.
     */
    std::size_t id = 0;

    VertexProperties() = default;
    VertexProperties(const llvm::Value *v);
  };

  /**
   * 	@brief Holds the information of an edge in the points-to graph.
   */
  struct EdgeProperties {
    /// This might be an Instruction, in particular a Call Instruction.
    const llvm::Value *value = nullptr;

    /// Holds the llvm IR code for that edge.
    std::string ir_code;

    /**
     * For an instruction the id is equal to the annotated id of the
     * instruction.
     * In all other cases it's zero.
     */
    std::size_t id = 0;

    EdgeProperties() = default;
    EdgeProperties(const llvm::Value *v);
  };

  /// Data structure for holding the points-to graph.
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
                                VertexProperties, EdgeProperties>
      graph_t;

  /// The type for vertex representative objects.
  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;

  /// The type for edge representative objects.
  typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;

  /// The type for a vertex iterator.
  typedef boost::graph_traits<graph_t>::vertex_iterator vertex_iterator_t;
  typedef boost::graph_traits<graph_t>::vertex_iterator vertex_iterator;
  typedef boost::graph_traits<graph_t>::out_edge_iterator out_edge_iterator;
  typedef boost::graph_traits<graph_t>::in_edge_iterator in_edge_iterator;

  /// Set of functions that allocate heap memory, e.g. new, new[], malloc.
  const static std::set<std::string> HeapAllocationFunctions;

private:
  struct allocation_site_dfs_visitor;
  struct reachability_dfs_visitor;

  /// The points to graph.
  graph_t ptg;
  std::map<const llvm::Value *, vertex_t> value_vertex_map;
  /// Keep track of what has already been merged into this points-to graph.
  std::set<std::string> ContainedFunctions;

public:
  /**
   * Creates a points-to graph based on the computed Alias results.
   *
   * @brief Creates a points-to graph for a given function.
   * @param AA Contains the computed Alias Results.
   * @param F Points-to graph is created for this particular function.
   * @param onlyConsiderMustAlias True, if only Must Aliases should be
   * considered.
   *                              False, if May and Must Aliases should be
   * considered.
   */
  PointsToGraph(llvm::AAResults &AA, llvm::Function *F,
                bool onlyConsiderMustAlias = false);

  /**
   * It is used when a points-to graph is restored from the database.
   *
   * @brief This will create an empty points-to graph, except the functions
   * names
   * that are contained in the points-to graph.
   * @param fnames Names of functions contained in the points-to graph.
   */
  PointsToGraph(std::vector<std::string> fnames);

  /**
   * @brief This will create an empty points-to graph. It is used when points-to
   * graphs are merged.
   */
  PointsToGraph() = default;

  virtual ~PointsToGraph() = default;

  /**
   * @brief Returns true if the given pointer is an interesting pointer,
   *        i.e. not a constant null pointer.
   */
  inline bool isInterestingPointer(llvm::Value *V);

  /**
   * @brief Returns a std::vector containing pointers which are escaping through
   *        function parameters.
   * @return Vector holding function argument pointers and the function argument
   * number.
   */
  std::vector<std::pair<unsigned, const llvm::Value *>>
  getPointersEscapingThroughParams();

  /**
   * @brief Returns a std::vector containing pointers which are escaping through
   *        function return statements.
   * @return Vector with pointers.
   */
  std::vector<const llvm::Value *> getPointersEscapingThroughReturns() const;

  /**
   * @brief Returns a std::vector containing pointers which are escaping through
   *        function return statements for a specific function.
   * @param F Function pointer
   * @return Vector with pointers.
   */
  std::vector<const llvm::Value *>
  getPointersEscapingThroughReturnsForFunction(const llvm::Function *Fd) const;

  /**
   * @brief Returns all reachable allocation sites from a given pointer.
   * @note An allocation site can either be an Alloca Instruction or a call to
   * an allocating function.
   * @return Set of Allocation sites.
   */
  std::set<const llvm::Value *>
  getReachableAllocationSites(const llvm::Value *V,
                              std::vector<const llvm::Instruction *> CallStack);

  /**
   * @brief Computes all possible types from a given std::set of allocation
   * sites.
   * @note An allocation site can either be an Alloca Instruction or a call to
   * an allocating function.
   * @param AS Set of Allocation site.
   * @return Set of Types.
   */
  std::set<const llvm::Type *>
  computeTypesFromAllocationSites(std::set<const llvm::Value *> AS);

  /**
   * @brief Checks if a given value is represented by a vertex in the points-to
   * graph.
   * @return True, the points-to graph contains the given value.
   */
  bool containsValue(llvm::Value *V);

  /**
   * @brief Computes the Points-to set for a given pointer.
   */
  std::set<const llvm::Value *> getPointsToSet(const llvm::Value *V);

  // TODO add more detailed description
  inline bool representsSingleFunction();
  void mergeWith(const PointsToGraph &Other, const llvm::Function *F);
  void mergeWith(const PointsToGraph &Other,
                 const std::vector<std::pair<llvm::ImmutableCallSite,
                                             const llvm::Function *>> &Calls);
  void mergeWith(PointsToGraph &Other, llvm::ImmutableCallSite CS,
                 const llvm::Function *F);

  /**
   * The value-vertex-map maps each Value of the points-to graph to
   * its corresponding Vertex in the points-to graph.
   *
   * @brief Prints the value-vertex-map to the command-line.
   */
  void printValueVertexMap();

  /**
   * @brief Prints the points-to graph to the command-line.
   */
  void print();

  /**
   * @brief Prints the points-to graph to the command-line.
   */
  void print() const;

  /**
   * @brief Prints the points-to graph as a .dot file.
   * @param filename Filename of the .dot file.
   */
  void printAsDot(const std::string &filename);

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();
  /**
   * @brief NOT YET IMPLEMENTED
   */
  json getAsJson();
};

} // namespace psr

#endif
