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
class ProjectIRCompiledDB;

class LLVMStructTypeHierarchy {
 public:
  struct VertexProperties {
    llvm::Type* llvmtype = nullptr;
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
  LLVMStructTypeHierarchy(const ProjectIRCompiledDB& IRDB);
  ~LLVMStructTypeHierarchy() = default;
  void analyzeModule(const llvm::Module& M);
  set<string> getTransitivelyReachableTypes(string TypeName);
  vector<const llvm::Function*> constructVTable(const llvm::Type* T,
                                                const llvm::Module* M);
  string getVTableEntry(string TypeName, unsigned idx);
  VTable getVTable(string TypeName);
  bool hasSuperType(string TypeName, string SuperTypeName);
  bool hasSubType(string TypeName, string SubTypeName);
  bool containsVTable(string TypeName);
  bool containsType(string TypeName);
  void printTransitiveClosure();
  void print();
  void printAsDot(const string& path="struct_type_hierarchy.dot");
	void exportPATBCJSON();
  // these are defined in the DBConn class
  friend void operator<<(DBConn& db, const LLVMStructTypeHierarchy& STH);
  friend void operator>>(DBConn& db, const LLVMStructTypeHierarchy& STH);
};

#endif /* ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_ */
