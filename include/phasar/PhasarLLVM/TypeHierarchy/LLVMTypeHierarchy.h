/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ClassHierarchy.h
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHY_H_
#define PHASAR_PHASARLLVM_TYPEHIERARCHY_LLVMTYPEHIERARCHY_H_

#include <iosfwd>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest_prod.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

#include <json.hpp>

#include <phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h>
#include <phasar/PhasarLLVM/TypeHierarchy/TypeHierarchy.h>

namespace llvm {
class Module;
class StructType;
class Function;
} // namespace llvm

namespace psr {

class ProjectIRDB;
/**
 * 	@brief Owns the class hierarchy of the analyzed program.
 *
 * 	This class is responsible for constructing a inter-modular class
 * 	hierarchy graph based on the data from the %ProjectIRCompiledDB
 * 	and reconstructing the virtual method tables.
 */
class LLVMTypeHierarchy
    : public TypeHierarchy<const llvm::StructType *, const llvm::Function *> {
public:
  struct VertexProperties {
    VertexProperties() = default;
    VertexProperties(const llvm::StructType *Type);
    const llvm::StructType *Type = nullptr;
    std::string TypeName = "";
    LLVMVFTable VFT;
    std::set<const llvm::StructType *> ReachableTypes;
  };

  /// Edges in the class hierarchy graph doesn't hold any additional
  /// information.
  struct EdgeProperties {
    EdgeProperties() = default;
  };

  /// Data structure holding the class hierarchy graph.
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                                VertexProperties, EdgeProperties>
      bidigraph_t;

  /// The type for vertex representative objects.
  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  /// The type for edge representative objects.
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;
  // Let us have some further handy typedefs.
  typedef boost::graph_traits<bidigraph_t>::vertex_iterator vertex_iterator_t;
  typedef boost::graph_traits<bidigraph_t>::out_edge_iterator
      out_edge_iterator_t;
  typedef boost::graph_traits<bidigraph_t>::in_edge_iterator in_edge_iterator_t;

private:
  bidigraph_t g;
  std::unordered_map<const llvm::StructType *, vertex_t> type_vertex_map;
  // maps type names to the corresponding vtable
  std::unordered_map<const llvm::StructType *, LLVMVFTable> type_vtbl_map;
  // holds all modules that are included in the type hierarchy
  std::unordered_set<const llvm::Module *> contained_modules;

  // void reconstructVTables(const llvm::Module &M);
  // FRIEND_TEST(VTableTest, SameTypeDifferentVTables);
  FRIEND_TEST(LTHTest, GraphConstruction);
  FRIEND_TEST(LTHTest, HandleLoadAndPrintOfNonEmptyGraph);

protected:
  // void buildLLVMTypeHierarchy(const llvm::Module &M);
  // void pruneTypeHierarchyWithVtable(const llvm::Function *constructor);

public:
  /**
   *  @brief Creates a LLVMStructTypeHierarchy based on the
   *         given ProjectIRCompiledDB.
   *  @param IRDB ProjectIRCompiledDB object.
   */
  LLVMTypeHierarchy(ProjectIRDB &IRDB);

  /**
   *  @brief Creates a LLVMStructTypeHierarchy based on the
   *         llvm::Module.
   *  @param M A llvm::Module.
   */
  // LLVMTypeHierarchy(const llvm::Module &M);

  ~LLVMTypeHierarchy() = default;

  /**
   * @brief Constructs the actual class hierarchy graph.
   * @param M LLVM module
   *
   * Extracts new information from the given module and adds new vertices
   * and edges accordingly to the type hierarchy graph.
   */
  void constructHierarchy(const llvm::Module &M);

  bool hasType(const llvm::StructType *Type) const override;

  bool isSubType(const llvm::StructType *Type,
                 const llvm::StructType *SubType) override;

  std::set<const llvm::StructType *>
  getReachableSubTypes(const llvm::StructType *Type) override;

  bool isSuperType(const llvm::StructType *Type,
                   const llvm::StructType *SuperType) override;

  std::set<const llvm::StructType *>
  getReachableSuperTypes(const llvm::StructType *Type) override;

  const llvm::StructType *getType(std::string TypeName) const override;

  std::set<const llvm::StructType *> getAllTypes() const override;

  std::string getTypeName(const llvm::StructType *Type) const override;

  bool hasVFTable(const llvm::StructType *Type) const override;

  LLVMVFTable *getVFTable(const llvm::StructType *Type) const override;

  size_t size() const override;

  bool empty() const override;

  void print(std::ostream &OS) const override;

  nlohmann::json getAsJson() const override;

  // void mergeWith(LLVMTypeHierarchy &Other);

  /**
   * 	@brief Prints the transitive closure of the class hierarchy graph.
   */
  // void printTransitiveClosure();

  /**
   * 	@brief Prints the class hierarchy to a .dot file.
   * 	@param path Path where the .dot file is created.
   */
  // void printAsDot(const std::string &path = "struct_type_hierarchy.dot");

  // void printGraphAsDot(std::ostream &out);

  // static bidigraph_t loadGraphFormDot(std::istream &in);
};

} // namespace psr

#endif
