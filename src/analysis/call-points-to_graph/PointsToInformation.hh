/*
 * PointsToGraph.hh
 *
 *  Created on: 08.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_POINTSTOINFORMATION_HH_
#define ANALYSIS_POINTSTOINFORMATION_HH_

#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/undirected_dfs.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
using namespace std;

// See the following llvm classes for comprehension
// http://llvm.org/docs/doxygen/html/AliasAnalysis_8cpp_source.html
// http://llvm.org/docs/doxygen/html/AliasAnalysisEvaluator_8cpp_source.html

// See also the different kinds of alias analyses
//	#include "llvm/Analysis/AliasAnalysis.h"
//	#include "llvm/Analysis/BasicAliasAnalysis.h"
//	#include "llvm/Analysis/CFG.h"
//	#include "llvm/Analysis/CFLAliasAnalysis.h"
//	#include "llvm/Analysis/CaptureTracking.h"
//	#include "llvm/Analysis/GlobalsModRef.h"
//	#include "llvm/Analysis/ObjCARCAliasAnalysis.h"
//	#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
//	#include "llvm/Analysis/ScopedNoAliasAA.h"
//	#include "llvm/Analysis/TargetLibraryInfo.h"
//	#include "llvm/Analysis/TypeBasedAliasAnalysis.h"

enum class AliasKind { MUST, MAY, ALL };

class PointsToInformation {
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

  struct formals_reachability_dfs_visitor : boost::default_dfs_visitor {
    set<vertex_t> formals;
    set<vertex_t>& aliasing_with_formals;
    formals_reachability_dfs_visitor(set<vertex_t> formals,
                                     set<vertex_t>& result)
        : formals(formals), aliasing_with_formals(result){};
    //		template<class Vertex, class Graph>
    //		void initialize_vertex(Vertex u, const Graph& g)
    //		{
    //			cout << "init_vertex" << endl;
    //		}
    //		template<class Vertex, class Graph>
    //		void start_vertex(Vertex u, const Graph& g)
    //		{
    //			cout << "start vertex" << endl;
    //		}
    //		template<class Vertex, class Graph>
    //		void discover_vertex(Vertex u, const Graph& g)
    //		{
    //			cout << "discover_vertex" << endl;
    //		}
    //	template<class Edge, class Graph>
    //	void examine_edge(Edge u, const Graph& g) {
    //		cout << "examine_edge" << endl;
    //	}
    //	template<class Edge, class Graph>
    //	void tree_edge(Edge u, const Graph& g) {
    //		cout << "tree_edge" << endl;
    //	}
    //	template<class Edge, class Graph>
    //	void back_edge(Edge u, const Graph& g) {
    //		cout << "back_edge" << endl;
    //	}
    //	template<class Edge, class Graph>
    //	void forward_or_cross_edge(Edge u, const Graph& g) {
    //		cout << "forward_cross_edge" << endl;
    //	}
    //	template<class Edge, class Graph>
    //	void finish_edge(Edge u, const Graph& g) {
    //		cout << "finish_edge" << endl;
    //	}
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

  AliasKind consideration;
  map<string, graph_t> function_ptg_map;
  map<const llvm::Value*, vertex_t> value_vertex_map;

 public:
  PointsToInformation(AliasKind cons = AliasKind::ALL);
  virtual ~PointsToInformation() = default;
  void analyzeModule(llvm::AAResults& AA, llvm::Module& M);
  inline bool isInterestingPointer(llvm::Value* V);
  set<const llvm::Value*> aliasWithFormalParameter(const llvm::Function* F,
                                                   const llvm::Value* V);
  set<const llvm::Value*> getPointsToSet(const llvm::Function* F,
                                         const llvm::Value* V);
  void print();
  void write_pti_graphviz(const string path, const llvm::Function* F);
  void printValueVertexMap();
};

#endif /* ANALYSIS_POINTSTOINFORMATION_HH_ */
