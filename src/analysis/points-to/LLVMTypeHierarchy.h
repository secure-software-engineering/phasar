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

#ifndef ANALYSIS_LLVMTYPEHIERARCHY_H_
#define ANALYSIS_LLVMTYPEHIERARCHY_H_

#include <algorithm>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <fstream>
#include "../../db/ProjectIRDB.h"
#include "../../utils/Logger.h"
#include "VTable.h"
#include <initializer_list>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
using namespace std;

  /**
   * 	@brief Owns the class hierarchy of the analyzed program.
   *
   * 	This class is responsible for constructing a inter-modular class
   * 	hierarchy graph based on the data from the %ProjectIRCompiledDB
   * 	and reconstructing the virtual method tables.
   */
class LLVMTypeHierarchy {
  friend class DBConn;

    /**
     * 	@brief Holds additional information for each vertex in the class
     * 	       hierarchy graph.
     */
    struct VertexProperties {
      llvm::Type* llvmtype = nullptr;
      /// always StructType so far - is it used anywhere???
      /// Name of the class/struct the vertex is representing.
      string name;
    };

    /// Edges in the class hierarchy graph doesn't hold any additional
    /// information.
    struct EdgeProperties {
      EdgeProperties() = default;
    };

    /// Data structure holding the class hierarchy graph.
    typedef boost::adjacency_list<boost::setS, boost::vecS,
                                  boost::bidirectionalS, VertexProperties,
                                  EdgeProperties>
        bidigraph_t;

    /// The type for vertex representative objects.
    typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;

    /// The type for edge representative objects.
    typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;

   private:
    struct reachability_dfs_visitor : boost::default_dfs_visitor {
      set<vertex_t>& subtypes;
      reachability_dfs_visitor(set<vertex_t>& types) : subtypes(types) {}
      template <typename Vertex, typename Graph>
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
    LLVMTypeHierarchy(ProjectIRDB& IRDB);

    ~LLVMTypeHierarchy() = default;

    /**
     * @brief Constructs the actual class hierarchy graph.
     * @param M LLVM module
     *
     * Extracts new information from the given module and adds new vertices
     * and edges accordingly to the type hierarchy graph.
     */
    void analyzeModule(const llvm::Module& M);

    /**
     * 	@brief Computes all types, which are transitiv reachable from
     * 	       the given type.
     * 	@param TypeName Name of the type.
     * 	@return Set of reachable types.
     */
    set<string> getTransitivelyReachableTypes(string TypeName);
    // not used?
    vector<const llvm::Function*> constructVTable(const llvm::Type* T,
                                                  const llvm::Module* M);

    /**
     * 	@brief Returns an entry at the given index from the VTable
     * 	       of the given type.
     * 	@param TypeName Type identifier.
     * 	@param idx Index in the VTable.
     * 	@return A function identifier.
     */
    string getVTableEntry(string TypeName, unsigned idx);

    /**
     * 	@brief Checks if one of the given types is a super-type of the
     * 	       other given type.
     * 	@param TypeName Type identifier.
     * 	@param SubTypeName Type identifier.
     * 	@return True, if the one type is a super-type of the other.
     * 	        False otherwise.
     *
     * 	NOT YET SUPPORTED!
     */
    bool hasSuperType(string TypeName, string SuperTypeName);

    VTable getVTable(string TypeName);

    /**
     * 	@brief Checks if one of the given types is a sub-type of the
     * 	       other given type.
     * 	@param TypeName Type identifier.
     * 	@param SubTypeName Type identifier.
     * 	@return True, if the one type is a sub-type of the other.
     * 	        False otherwise.
     */
    bool hasSubType(string TypeName, string SubTypeName);

    /**
     *	@brief Checks if the given type has a virtual method table.
     *	@param TypeName Type identifier.
     *	@return True, if the given type has a virtual method table.
     *	        False otherwise.
     */
    bool containsVTable(string TypeName) const;

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
    void printAsDot(const string& path = "struct_type_hierarchy.dot");

    bool containsType(string TypeName);

    string getPlainTypename(string TypeName);

    json exportPATBCJSON();

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

#endif /* ANALYSIS_LLVMTYPEHIERARCHY_HH_ */
