/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOGRAPH_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOGRAPH_H_

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "boost/graph/adjacency_list.hpp"

#include "llvm/IR/AbstractCallSite.h"

#include "nlohmann/json.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasedPointsToAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Pointer/PointsToSetOwner.h"

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
 * 	This class is a representation of a points-to graph. It is possible to
 * 	construct a points-to graph for a single function using the results of
 *  the llvm alias analysis or merge several points-to graphs into a single
 *	points-to graph, e.g. to construct a whole program points-to graph.
 *
 *	The graph itself is undirectional and can have labeled edges.
 *
 *	@brief Represents the points-to graph of a function.
 */
class LLVMPointsToGraph : public LLVMPointsToInfo {
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
    VertexProperties(const llvm::Value *V);
    std::string getValueAsString() const;

    // Fetching the users for V is expensive, so we cache the result.
    mutable std::vector<const llvm::User *> Users;
    std::vector<const llvm::User *> getUsers() const;
  };

  /**
   * 	@brief Holds the information of an edge in the points-to graph.
   */
  struct EdgeProperties {
    /// This may contain a call or invoke instruction.
    const llvm::Value *V = nullptr;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Value *V);
    [[nodiscard]] std::string getValueAsString() const;
  };

  /// Data structure for holding the points-to graph.
  using graph_t =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                            VertexProperties, EdgeProperties>;

  /// The type for vertex representative objects.
  using vertex_t = boost::graph_traits<graph_t>::vertex_descriptor;

  /// The type for edge representative objects.
  using edge_t = boost::graph_traits<graph_t>::edge_descriptor;

  /// The type for a vertex iterator.
  using vertex_iterator = boost::graph_traits<graph_t>::vertex_iterator;
  using out_edge_iterator = boost::graph_traits<graph_t>::out_edge_iterator;
  using in_edge_iterator = boost::graph_traits<graph_t>::in_edge_iterator;

private:
  struct AllocationSiteDFSVisitor;
  struct ReachabilityDFSVisitor;

  /// The points to graph.
  graph_t PAG;
  using ValueVertexMapT = std::unordered_map<const llvm::Value *, vertex_t>;
  ValueVertexMapT ValueVertexMap;
  /// Keep track of what has already been merged into this points-to graph.
  std::unordered_set<const llvm::Function *> AnalyzedFunctions;
  LLVMBasedPointsToAnalysis PTA;
  PointsToSetOwner<PointsToSetTy> Owner;
  std::unordered_map<const llvm::Value *, DynamicPointsToSetPtr<PointsToSetTy>>
      Cache;

  // void mergeGraph(const LLVMPointsToGraph &Other);

  void computePointsToGraph(const llvm::Value *V);

  void computePointsToGraph(llvm::Function *F);

public:
  /**
   * Creates a points-to graph based on the computed Alias results.
   *
   * @brief Creates a points-to graph for a given function.
   * @param AA Contains the computed Alias Results.
   * @param F Points-to graph is created for this particular function.
   * @param onlyConsiderMustAlias True, if only Must Aliases should be
   * considered. False, if May and Must Aliases should be
   * considered.
   */
  LLVMPointsToGraph(ProjectIRDB &IRDB, bool UseLazyEvaluation = true,
                    PointerAnalysisType PATy = PointerAnalysisType::CFLAnders);

  ~LLVMPointsToGraph() override = default;

  bool isInterProcedural() const override;

  PointerAnalysisType getPointerAnalysistype() const override;

  AliasResult alias(const llvm::Value *V1, const llvm::Value *V2,
                    const llvm::Instruction *I = nullptr) override;

  PointsToSetPtrTy
  getPointsToSet(const llvm::Value *V,
                 const llvm::Instruction *I = nullptr) override;

  AllocationSiteSetPtrTy
  getReachableAllocationSites(const llvm::Value *V, bool IntraProcOnly = false,
                              const llvm::Instruction *I = nullptr) override;

  [[nodiscard]] bool
  isInReachableAllocationSites(const llvm::Value *V,
                               const llvm::Value *PotentialValue,
                               bool IntraProcOnly = false,
                               const llvm::Instruction *I = nullptr) override;

  void mergeWith(const PointsToInfo<const llvm::Value *,
                                    const llvm::Instruction *> &PTI) override;

  void introduceAlias(const llvm::Value *V1, const llvm::Value *V2,
                      const llvm::Instruction *I = nullptr,
                      AliasResult Kind = AliasResult::MustAlias) override;

  void print(std::ostream &OS = std::cout) const override;

  nlohmann::json getAsJson() const override;

  void printAsJson(std::ostream &OS = std::cout) const override;

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
   * @brief Checks if a given value is represented by a vertex in the points-to
   * graph.
   * @return True, the points-to graph contains the given value.
   */
  bool containsValue(llvm::Value *V);

  /**
   * The value-vertex-map maps each Value of the points-to graph to
   * its corresponding Vertex in the points-to graph.
   *
   * @brief Prints the value-vertex-map to the command-line.
   */
  void printValueVertexMap();

  class PointerVertexOrEdgePrinter {
  public:
    PointerVertexOrEdgePrinter(const graph_t &PAG) : PAG(PAG) {}
    template <class VertexOrEdge>
    void operator()(std::ostream &Out, const VertexOrEdge &V) const {
      Out << "[label=\"" << PAG[V].getValueAsString() << "\"]";
    }

  private:
    const graph_t &PAG;
  };

  static inline PointerVertexOrEdgePrinter
  makePointerVertexOrEdgePrinter(const graph_t &PAG) {
    return {PAG};
  }

  /**
   * @brief Prints the points-to graph in .dot format to the given output
   * stream.
   * @param outputstream.
   */
  void printAsDot(std::ostream &OS = std::cout) const;

  size_t getNumVertices() const;

  size_t getNumEdges() const;
};

} // namespace psr

#endif
