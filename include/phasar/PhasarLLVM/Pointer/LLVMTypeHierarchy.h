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

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMTYPEHIERARCHY_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMTYPEHIERARCHY_H_

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

#include <phasar/PhasarLLVM/Pointer/VTable.h>

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
class LLVMTypeHierarchy {
public:
  /// necessary for storing/loading the LLVMTypeHierarchy to/from database
  friend class DBConn;
  using json = nlohmann::json;

  struct VertexProperties {
    VertexProperties() = default;
    VertexProperties(llvm::StructType *Type, std::string TypeName);
    llvm::StructType *llvmtype = nullptr;
    /// Name of the class/struct the vertex is representing.
    std::string name;
    VTable vtbl;
    std::set<std::string> reachableTypes;
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
  std::unordered_map<std::string, vertex_t> type_vertex_map;
  // maps type names to the corresponding vtable
  std::unordered_map<std::string, VTable> type_vtbl_map;
  // holds all modules that are included in the type hierarchy
  std::unordered_set<const llvm::Module *> contained_modules;

  void reconstructVTables(const llvm::Module &M);
  // FRIEND_TEST(VTableTest, SameTypeDifferentVTables);
  FRIEND_TEST(LTHTest, GraphConstruction);
  FRIEND_TEST(LTHTest, HandleLoadAndPrintOfNonEmptyGraph);

protected:
  void buildLLVMTypeHierarchy(const llvm::Module &M);
  void pruneTypeHierarchyWithVtable(const llvm::Function *constructor);

public:
  /**
   * 	@brief Creates an empty LLVMStructTypeHierarchy.
   *
   * 	Is used, when re-storing type hierarchy from database.
   */
  LLVMTypeHierarchy() = default;

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
  LLVMTypeHierarchy(const llvm::Module &M);

  ~LLVMTypeHierarchy() = default;

  /**
   * @brief Constructs the actual class hierarchy graph.
   * @param M LLVM module
   *
   * Extracts new information from the given module and adds new vertices
   * and edges accordingly to the type hierarchy graph.
   */
  void constructHierarchy(const llvm::Module &M);

  /**
   * 	@brief Computes all types, which are transitiv reachable from
   * 	       the given type.
   * 	@param TypeName Name of the type.
   * 	@return Set of reachable types.
   */
  std::set<std::string> getTransitivelyReachableTypes(std::string TypeName);

  /**
   * 	@brief Returns an entry at the given index from the VTable
   * 	       of the given type.
   * 	@param TypeName Type identifier.
   * 	@param idx Index in the VTable.
   * 	@return A function identifier.
   */
  std::string getVTableEntry(std::string TypeName, unsigned idx) const;

  /**
   * 	@brief Checks if one of the given types is a super-type of the
   * 	       other given type.
   * 	@param TypeName Type identifier.
   * 	@param SubTypeName Type identifier.
   * 	@return True, if the one type is a super-type of the other.
   * 	        False otherwise.
   */
  bool hasSuperType(std::string TypeName, std::string SuperTypeName);

  VTable getVTable(std::string TypeName) const;

  /**
   * 	@brief Checks if one of the given types is a sub-type of the
   * 	       other given type.
   * 	@param TypeName Type identifier.
   * 	@param SubTypeName Type identifier.
   * 	@return True, if the one type is a sub-type of the other.
   * 	        False otherwise.
   */
  bool hasSubType(std::string TypeName, std::string SubTypeName);

  /**
   *	@brief Checks if the given type has a virtual method table.
   *	@param TypeName Type identifier.
   *	@return True, if the given type has a virtual method table.
   *	        False otherwise.
   */
  bool containsVTable(std::string TypeName) const;

  /**
   *	@brief Returns the number of types.
   *	@return Number of user-defined types.
   */
  size_t getNumTypes() const;

  /**
   *	@brief Returns the number of vtable entries for a given type.
   *	@return Number of vtable entries.
   */
  size_t getNumVTableEntries(std::string TypeName) const;

  /**
   *	@brief Returns a pointer to the llvm struct type.
   *  @param TypeName Type identifier
   *	@return Pointer to llvm::StructType.
   */
  const llvm::StructType *getType(std::string TypeName) const;

  void mergeWith(LLVMTypeHierarchy &Other);

  /**
   * 	@brief Prints the transitive closure of the class hierarchy graph.
   */
  void printTransitiveClosure();

  /**
   * 	@brief Prints the class hierarchy to the command-line.
   */
  void print();
  /**
   * 	@brief Prints the class hierarchy to a .dot file.
   * 	@param path Path where the .dot file is created.
   */
  void printAsDot(const std::string &path = "struct_type_hierarchy.dot");

  bool containsType(std::string TypeName) const;

  void addVTableEntry(std::string TypeName, std::string FunctionName);

  void printGraphAsDot(std::ostream &out);

  static bidigraph_t loadGraphFormDot(std::istream &in);

  json getAsJson();

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  // these are defined in the DBConn class
  /**
   * 	@brief %LLVMStructTypeHierarchy store operator.
   * 	@param db SQLite3 database to store the class hierarchy in.
   * 	@param STH %LLVMStructTypeHierarchy object that is stored.
   *
   * 	By storing the class hierarchy in the database, a repeated
   * 	reconstruction of the class hierarchy graph as well as the
   * 	VTables from the corresponding LLVM module(s) is unnecessary.
   *
   * 	To store the class hierarchy graph itself, a %Hexastore data
   * 	structure is used.
   */
  // friend void operator<<(DBConn& db, const LLVMTypeHierarchy& STH);

  /**
   * 	@brief %LLVMStructTypeHierarchy load operator.
   * 	@param db SQLite3 database the class hierarchy is stored in.
   * 	@param STH %LLVMStructTypeHierarchy object that is restored.
   */
  // friend void operator>>(DBConn& db, LLVMTypeHierarchy& STH);
};

} // namespace psr

#endif
