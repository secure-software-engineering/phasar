/*
 * ClassHierarchy.h
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_LLVMTYPEHIERARCHY_H_
#define ANALYSIS_LLVMTYPEHIERARCHY_H_

#include "../../db/ProjectIRDB.h"
#include "../../utils/Logger.h"
#include "VTable.h"
#include <algorithm>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <fstream>
#include <gtest/gtest_prod.h>
#include <initializer_list>
#include <iostream>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
using namespace std;

class LLVMTypeHierarchy {
public:
  /// necessary for storing/loading the LLVMTypeHierarchy to/from database
  friend class DBConn;

  struct VertexProperties {
    llvm::Type *llvmtype = nullptr;
    string name;
  };

  struct EdgeProperties {
    EdgeProperties() = default;
  };

  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                                VertexProperties, EdgeProperties>
      bidigraph_t;
  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;

private:
  struct reachability_dfs_visitor : boost::default_dfs_visitor {
    set<vertex_t> &subtypes;
    reachability_dfs_visitor(set<vertex_t> &types) : subtypes(types) {}
    template <class Vertex, class Graph>
    void finish_vertex(Vertex u, const Graph &g) {
      subtypes.insert(u);
    }
  };

  bidigraph_t g;
  map<string, vertex_t> type_vertex_map;
  // maps type names to the corresponding vtable
  map<string, VTable> vtable_map;
  set<string> recognized_struct_types;
  // holds all modules that are included in the type hierarchy
  set<const llvm::Module *> contained_modules;

  void reconstructVTable(const llvm::Module &M);
  FRIEND_TEST(VTableTest, SameTypeDifferentVTables);

public:
  LLVMTypeHierarchy() = default;
  LLVMTypeHierarchy(ProjectIRDB &IRDB);
  ~LLVMTypeHierarchy() = default;
  void analyzeModule(const llvm::Module &M);
  set<string> getTransitivelyReachableTypes(string TypeName);
  vector<const llvm::Function *> constructVTable(const llvm::Type *T,
                                                 const llvm::Module *M);
  string getVTableEntry(string TypeName, unsigned idx);
  VTable getVTable(string TypeName);
  bool hasSuperType(string TypeName, string SuperTypeName);
  bool hasSubType(string TypeName, string SubTypeName);
  bool containsVTable(string TypeName);
  bool containsType(string TypeName);
  void printTransitiveClosure();
  string getPlainTypename(string TypeName);
  void print();
  void printGraphAsDot(ostream& out);
  static bidigraph_t loadGraphFormDot(istream& in);
  json exportPATBCJSON();
};

#endif /* ANALYSIS_LLVMTYPEHIERARCHY_HH_ */
