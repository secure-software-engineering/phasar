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
#include <llvm/IR/Metadata.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../../lib/GraphExtensions.hh"
#include "../../utils/utils.hh"
#include "../../utils/Configuration.hh"
#include "../../db/DBConn.hh"
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

// forward declare the DBConn class
class DBConn;

class PointsToGraph {
public:
  struct VertexProperties {
    const llvm::Value* value = nullptr;
    string ir_code;
    size_t id = 0;
  	VertexProperties() = default;
  	VertexProperties(llvm::Value* v);
  };

  struct EdgeProperties {
    const llvm::Value* value = nullptr;
    string ir_code;
    size_t id = 0;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Value* v);
  };

  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
                                VertexProperties, EdgeProperties>
      graph_t;
  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;
  typedef boost::graph_traits<graph_t>::vertex_iterator vertex_iterator_t;

  // new, new[], malloc
  const static set<string> allocating_functions;

private:
  struct allocation_site_dfs_visitor : boost::default_dfs_visitor {
  	set<const llvm::Value*>& allocation_sites;
  	allocation_site_dfs_visitor(set<const llvm::Value*>& allocation_sizes)
  			: allocation_sites(allocation_sizes) {}
  	template <class Vertex, class Graph>
  	void finish_vertex(Vertex u, const Graph& g) {
  		if (const llvm::AllocaInst* alloc = llvm::dyn_cast<llvm::AllocaInst>(g[u].value)) {
  			allocation_sites.insert(g[u].value);
  		}
  		if (const llvm::CallInst* cs = llvm::dyn_cast<llvm::CallInst>(g[u].value)) {
  			if (cs->getCalledFunction() != nullptr &&
  					allocating_functions.find(cs->getCalledFunction()->getName().str()) != allocating_functions.end()) {
  				allocation_sites.insert(g[u].value);
  			}
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
  vector<string> merge_stack;

public:
  PointsToGraph(llvm::AAResults& AA, llvm::Function* F, bool onlyConsiderMustAlias=false);
  PointsToGraph() = default;
  virtual ~PointsToGraph() = default;
  inline bool isInterestingPointer(llvm::Value* V);
  vector<pair<unsigned, const llvm::Value*>> getPointersEscapingThroughParams();
  vector<const llvm::Value*> getPointersEscapingThroughReturns();
  set<const llvm::Value*> getReachableAllocationSites(const llvm::Value* V);
  set<const llvm::Type*> computeTypesFromAllocationSites(set<const llvm::Value*> AS);
  bool containsValue(llvm::Value* V);
  set<const llvm::Value*> getPointsToSet(const llvm::Value* V);
  void mergeWith(PointsToGraph& other,
  							 vector<pair<const llvm::Value*, const llvm::Value*>> v_in_first_u_in_second,
								 const llvm::Value* callsite_value);
  void printValueVertexMap();
  void print();
  void printAsDot(const string& filename);
  // these are defined in the DBConn class
  friend void operator<<(DBConn& db, const PointsToGraph& STH);
  friend void operator>>(DBConn& db, const PointsToGraph& STH);
};

#endif /* ANALYSIS_POINTSTOGRAPH_HH_ */
