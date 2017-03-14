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
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <vector>
using namespace std;

// struct my_dfs_visitor : boost::default_dfs_visitor {
//	// psssh, don't ask questions ...
//	//set<vertex_t>* collected_vertices = new set<vertex_t>;
//
//	template< typename Edge, typename Graph >
//	void tree_edge(Edge e, const Graph & g) const
//	{
//	//	collected_vertices->insert(boost::target(e, g));
//		cout << "found tree edge" << endl;
//	}
//
//};

class LLVMStructTypeHierarchy {
 private:
  struct VertexProperties {
    llvm::Type* llvmtype;
    string name;
  };

  struct EdgeProperties {
    string name;
  };

  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::directedS,
                                VertexProperties, EdgeProperties>
      digraph_t;
  typedef boost::graph_traits<digraph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<digraph_t>::edge_descriptor edge_t;

  struct reachability_dfs_visitor : boost::default_dfs_visitor {
    set<vertex_t>& subtypes;
    reachability_dfs_visitor(set<vertex_t>& types) : subtypes(types) {}
    template <class Vertex, class Graph>
    void finish_vertex(Vertex u, const Graph& g) {
      subtypes.insert(u);
    }
  };

  digraph_t g;
  map<const llvm::Type*, vertex_t> type_vertex_map;

 public:
  LLVMStructTypeHierarchy() = default;
  ~LLVMStructTypeHierarchy() = default;
  void analyzeModule(const llvm::Module& M);
  set<const llvm::Type*> getTransitivelyReachableTypes(const llvm::Type* T);
  vector<const llvm::Function*> constructVTable(const llvm::Type* T,
                                                const llvm::Module* M);
  const llvm::Function* getFunctionFromVirtualCallSite(
      llvm::Module* M, llvm::ImmutableCallSite ICS);
  bool containsSubType(const llvm::Type* T, const llvm::Type* ST);
  bool hasSuperType(const llvm::Type* ST, const llvm::Type* T);
  bool hasSubType(const llvm::Type* ST, const llvm::Type* T);
  bool containsVTable(const llvm::Type* T);
  void printTransitiveClosure();
  friend ostream& operator<<(ostream& os, const LLVMStructTypeHierarchy& ch);
};

#endif /* ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_ */
