/*
 * PointsToGraph.hh
 *
 *  Created on: 08.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_POINTSTOGRAPH_HH_
#define ANALYSIS_POINTSTOGRAPH_HH_

#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

void PrintResults(const char* Msg, bool P, const llvm::Value* V1,
                  const llvm::Value* V2, const llvm::Module* M);

inline void PrintModRefResults(const char* Msg, bool P,
                               const llvm::Instruction* I,
                               const llvm::Value* Ptr, const llvm::Module* M);

inline void PrintModRefResults(const char* Msg, bool P,
                               const llvm::CallSite CSA,
                               const llvm::CallSite CSB, const llvm::Module* M);

inline void PrintLoadStoreResults(const char* Msg, bool P,
                                  const llvm::Value* V1, const llvm::Value* V2,
                                  const llvm::Module* M);

class PointsToGraph {
 private:
  struct VertexProperties {
    llvm::Value* value;
    string ir;
  };

  struct EdgeProperties {
    string name;
  };

  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
                                VertexProperties, EdgeProperties>
      graph_t;
  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;
  typedef boost::graph_traits<graph_t>::vertex_iterator vertex_iterator_t;

  struct formals_reachability_dfs_visitor : boost::default_dfs_visitor {
    set<vertex_t> formals;
    set<vertex_t>& aliasing_with_formals;
    formals_reachability_dfs_visitor(set<vertex_t> formals,
                                     set<vertex_t>& result)
        : formals(formals), aliasing_with_formals(result){};
    template <class Vertex, class Graph>
    void finish_vertex(Vertex u, const Graph& g) {
      if (formals.find(u) != formals.end()) {
        aliasing_with_formals.insert(u);
      }
    }
  };

  struct reachability_dfs_visitor : boost::default_dfs_visitor {
    set<vertex_t>& points_to_set;
    reachability_dfs_visitor(set<vertex_t>& result) : points_to_set(result) {}
    template <class Vertex, class Graph>
    void finish_vertex(Vertex u, const Graph& g) {
      points_to_set.insert(u);
    }
  };

  graph_t ptg;
  map<const llvm::Value*, vertex_t> value_vertex_map;
  llvm::Function& F;

 public:
  PointsToGraph(llvm::AAResults& AA, llvm::Function* F);
  PointsToGraph() = default;
  virtual ~PointsToGraph() = default;
  inline bool isInterestingPointer(llvm::Value* V);
  bool containsValue(llvm::Value* V);
  set<const llvm::Value*> isAliasingWithFormals(const llvm::Value* V);
  set<const llvm::Value*> getPointsToSet(const llvm::Value* V);
  void printValueVertexMap();
  void merge_graphs(PointsToGraph& g, const llvm::Value* v_in_g,
                    const PointsToGraph& h, const llvm::Value* u_in_h);
  void print();
};

#endif /* ANALYSIS_POINTSTOGRAPH_HH_ */
