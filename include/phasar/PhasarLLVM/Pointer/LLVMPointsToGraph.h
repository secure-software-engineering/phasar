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

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>

#include <llvm/IR/CallSite.h>

#include <nlohmann/json.hpp>

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

/**
 * @brief Returns true if the given pointer is an interesting pointer,
 *        i.e. not a constant null pointer.
 */
static inline bool isInterestingPointer(llvm::Value *V) {
  return V->getType()->isPointerTy() &&
         !llvm::isa<llvm::ConstantPointerNull>(V);
}

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
     * Variable or a formal Argument.
     */
    const llvm::Value *V = nullptr;
    VertexProperties() = default;
    VertexProperties(const llvm::Value *v);
    std::string getValueAsString() const;

    // Fetching the users for V is expensive, so we cache the result.
    mutable std::vector<const llvm::User *> users;
    std::vector<const llvm::User *> getUsers() const;
  };

  /**
   * 	@brief Holds the information of an edge in the points-to graph.
   */
  struct EdgeProperties {
    /// This may contain a call or invoke instruction.
    const llvm::Value *V = nullptr;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Value *v);
    std::string getValueAsString() const;
  };

  /// Data structure for holding the points-to graph.
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                VertexProperties, EdgeProperties>
      graph_t;

  /// The type for vertex representative objects.
  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;

  /// The type for edge representative objects.
  typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;

  /// The type for a vertex iterator.
  typedef boost::graph_traits<graph_t>::vertex_iterator vertex_iterator;
  typedef boost::graph_traits<graph_t>::out_edge_iterator out_edge_iterator;
  typedef boost::graph_traits<graph_t>::in_edge_iterator in_edge_iterator;

  /// Set of functions that allocate heap memory, e.g. new, new[], malloc.
  inline const static std::set<std::string> HeapAllocationFunctions = {
      "_Znwm", "_Znam", "malloc", "calloc", "realloc"};

private:
  struct AllocationSiteDFSVisitor;
  struct ReachabilityDFSVisitor;

  /// The points to graph.
  graph_t PAG;
  typedef std::unordered_map<const llvm::Value *, vertex_t> ValueVertexMapT;
  ValueVertexMapT ValueVertexMap;
  /// Keep track of what has already been merged into this points-to graph.
  std::unordered_set<const llvm::Function *> ContainedFunctions;

  void mergeGraph(const PointsToGraph &Other);

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
  PointsToGraph(llvm::Function *F, llvm::AAResults &AA);

  /**
   * @brief This will create an empty points-to graph. It is used when points-to
   * graphs are merged.
   */
  PointsToGraph() = default;

  virtual ~PointsToGraph() = default;

  /**
   * @brief Returns true if graph contains 0 nodes.
   */
  bool empty() const;

  /**
   * @brief Returns the number of graph nodes.
   */
  size_t size() const;

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
  std::set<const llvm::Value *> getPointsToSet(const llvm::Value *V) const;

  // TODO add more detailed description
  inline bool representsSingleFunction();
  void mergeWith(const PointsToGraph *Other, const llvm::Function *F);

  void mergeCallSite(const llvm::ImmutableCallSite &CS,
                     const llvm::Function *F);

  void mergeWith(const PointsToGraph &Other,
                 const std::vector<std::pair<llvm::ImmutableCallSite,
                                             const llvm::Function *>> &Calls);

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
  void print(std::ostream &OS = std::cout) const;

  class PointerVertexOrEdgePrinter {
  public:
    PointerVertexOrEdgePrinter(const graph_t &PAG) : PAG(PAG) {}
    template <class VertexOrEdge>
    void operator()(std::ostream &out, const VertexOrEdge &v) const {
      out << "[label=\"" << PAG[v].getValueAsString() << "\"]";
    }

  private:
    const graph_t &PAG;
  };

  static inline PointerVertexOrEdgePrinter
  makePointerVertexOrEdgePrinter(const graph_t &PAG) {
    return PointerVertexOrEdgePrinter(PAG);
  }

  /**
   * @brief Prints the points-to graph in .dot format to the given output
   * stream.
   * @param outputstream.
   */
  void printAsDot(std::ostream &OS = std::cout) const;

  size_t getNumVertices() const;

  size_t getNumEdges() const;
  /**
   * @brief NOT YET IMPLEMENTED
   */
  nlohmann::json getAsJson();
};

} // namespace psr

#endif
