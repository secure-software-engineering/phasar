/*
 * ClassHierarchy.hh
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_
#define ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <algorithm>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <tuple>
#include <vector>
#include "../../db/DBConn.hh"
#include "../../db/ProjectIRCompiledDB.hh"
#include "VTable.hh"
using namespace std;

class DBConn;

/**
 * 	@brief Represents/owns the class hierarchy of the analyzed program.
 *
 * 	This class is responsible for constructing a inter-modular class hierarchy graph based on the
 * 	data from the %ProjectIRCompiledDB and reconstructing the virtual function tables.
 */
class LLVMStructTypeHierarchy {
 public:
  /// Additional information for each vertex in the class hierarchy graph
  struct VertexProperties {
  	/// always StructType so far
    llvm::Type* llvmtype;
    /// Name of the class type the vertex is representing
    string name;
  };

  struct EdgeProperties {
    EdgeProperties() = default;
  };

  /// Data structure holding the class hierarchy graph
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                                VertexProperties, EdgeProperties>
      bidigraph_t;


  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;

 private:
  struct reachability_dfs_visitor : boost::default_dfs_visitor {
    set<vertex_t>& subtypes;
    reachability_dfs_visitor(set<vertex_t>& types) : subtypes(types) {}
    template <class Vertex, class Graph>
    void finish_vertex(Vertex u, const Graph& g) {
      subtypes.insert(u);
    }
  };

  bidigraph_t g;
  map<string, vertex_t> type_vertex_map;
  // maps type names to the corresponding vtable
  map<string, VTable> vtable_map;
  set<string> recognized_struct_types;

  void reconstructVTable(const llvm::Module& M);

 public:
  LLVMStructTypeHierarchy() = default;

  /**
   *  @brief Creates a LLVMStructTypeHierarchy based on the given ProjectIRCompiledDB.
   *  @param IRDB ProjectIRCompiledDB object.
   */
  LLVMStructTypeHierarchy(const ProjectIRCompiledDB& IRDB);

  ~LLVMStructTypeHierarchy() = default;

  /**
   * @brief Constructs the actual class hierarchy graph.
   * @param M LLVM module
   *
   * Extracts new information from the given module and adds new vertices and edges
   * to the type hierarchy graph. Also creates the type_vertex_map and fills the
   * recognized_struct_types set.
   */
  void analyzeModule(const llvm::Module& M);
  set<string> getTransitivelyReachableTypes(string TypeName);
  // not used?
  vector<const llvm::Function*> constructVTable(const llvm::Type* T,
                                                const llvm::Module* M);
  string getVTableEntry(string TypeName, unsigned idx);
  bool hasSuperType(string TypeName, string SuperTypeName);
  bool hasSubType(string TypeName, string SubTypeName);
  bool containsVTable(string TypeName) const;
  void printTransitiveClosure();

  /**
   * 	@brief Prints the class hierarchy to the command-line.
   */
  void print();

  /**
   * 	@brief Prints the class hierarchy to a .dot file.
   * 	@param path Path where the .dot file is created.
   */
  void printAsDot(const string& path="struct_type_hierarchy.dot");

  // these are defined in the DBConn class
  /**
   * 	@brief %LLVMStructTypeHierarchy store operator.
   * 	@param db SQLite3 database to store the class hierarchy in.
   * 	@param STH %LLVMStructTypeHierarchy object that is stored.
   *
   * 	By storing the class hierarchy in the database, a repeated reconstruction of the class
   * 	hierarchy graph as well as the VTables from the corresponding LLVM module(s) is unnecessary.
   *
   * 	To store the class hierarchy graph itself, a %Hexastore data structure is used.
   */
  friend void operator<<(DBConn& db, const LLVMStructTypeHierarchy& STH);

  /**
   * 	@brief %LLVMStructTypeHierarchy load operator.
   * 	@param db SQLite3 database the class hierarchy is stored in.
   * 	@param STH %LLVMStructTypeHierarchy object that is restored.
   */
  friend void operator>>(DBConn& db, LLVMStructTypeHierarchy& STH);
};

#endif /* ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_ */
