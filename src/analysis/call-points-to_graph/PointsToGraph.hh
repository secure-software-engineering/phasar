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

/**
 * 	@brief Represents the points-to graph of a function.
 *
 *	It is possible to construct a points-to graph for a single function using the results of the
 *	llvm alias analysis or merge several points-to graphs into a single points-to graph, e.g. to
 *	construct a whole program points-to graph.
 *
 *	The graph itself is undirectional and can have labeled edges.
 */
class PointsToGraph {
public:
	/**
	 * 	@brief Holds the information of a vertex in the points-to graph.
	 */
  struct VertexProperties {
		/**
		 * 	This might be a llvm::Instruction (or an operand of an instruction), llvm::GlobalValue,
		 * 	llvm::Argument or an operand.
		 */
		const llvm::Value* value = nullptr;
		/// Holds the llvm IR code for that vertex.
    string ir_code;
    /**
     *  For an instruction the id is equal to the annotated id of the instruction.
     *  In all other cases it's zero.
     */
		size_t id = 0;
  	VertexProperties() = default;
  	VertexProperties(const llvm::Value* v);
  };

	/**
	 * 	@brief Holds the information of an edge in the points-to graph.
	 */
  struct EdgeProperties {
    const llvm::Value* value = nullptr;
    string ir_code;
    size_t id = 0;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Value* v);
  };

	/// Data structure for holding the points-to graph.
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
                                VertexProperties, EdgeProperties>
      graph_t;

	/// The type for vertex representative objects.
  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;

	/// The type for edge representative objects.
	typedef boost::graph_traits<graph_t>::edge_descriptor edge_t;

	/// The type for a vertex iterator.
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
  /**
   * @brief Creates a points-to graph for a given function.
   *
   * Creates a points-to graph based on the computed Alias Results.
   *
   * @param AA Contains the computed Alias Results.
   * @param F Points-to graph is created for this particular function.
   * @param onlyConsiderMustAlias True, if only Must Aliases should be considered.
   *                              False, if May and Must Aliases should be considered.
   */
  PointsToGraph(llvm::AAResults& AA, llvm::Function* F, bool onlyConsiderMustAlias=false);

	/**
	 * This will create an empty points-to graph, except the functions names that are contained in the
	 * points-to graph. It is used when a points-to graph is restored from the database.
	 *
	 * @param fnames Names of functions contained in the points-to graph.
	 */
  PointsToGraph(vector<string> fnames) : merge_stack(fnames) {};

  /**
   * This will create an empty points-to graph. It is used when points-to graphs are merged.
   */
  PointsToGraph() = default;

  virtual ~PointsToGraph() = default;

  /**
   * @brief Returns true if the given pointer is an interesting pointer.
   *
   * An pointer is interesting, if it's not a constant null pointer.
   */
  inline bool isInterestingPointer(llvm::Value* V);

  /**
   * @brief Returns a vector containing pointers which are escaping through
   *        function parameters.
   * @return Vector holding function argument pointers and the function argument number.
   */
  vector<pair<unsigned, const llvm::Value*>> getPointersEscapingThroughParams();

  /**
   * @brief Returns a vector containing pointers which are escaping through
   *        function return statements.
   * @return Vector with pointers.
   */
  vector<const llvm::Value*> getPointersEscapingThroughReturns();

  /**
   * @brief Returns all reachable allocation sites from a given pointer.
   *
   * An allocation site can either be an AllocaInst or a call to an allocating function.
   *
   * @return Set of allocation sites.
   */
  set<const llvm::Value*> getReachableAllocationSites(const llvm::Value* V);

  /**
   * @brief Computes all possible types from a given set of allocation sites.
   *
   * An allocation site can either be an AllocaInst or a call to an allocating function.
   *
   * @param AS Set of allocation site.
   * @return Set of Types.
   */
  set<const llvm::Type*> computeTypesFromAllocationSites(set<const llvm::Value*> AS);

  /**
   * @brief Checks if a given value is represented by a vertex in the points-to graph.
   * @return True, the points-to graph contains the given value.
   */
  bool containsValue(llvm::Value* V);

  /**
   *
   * @param V
   * @return
   */
  set<const llvm::Value*> getPointsToSet(const llvm::Value* V);

  /**
   *
   * @param other
   * @param v_in_first_u_in_second
   * @param callsite_value
   */
  void mergeWith(PointsToGraph& other,
  							 vector<pair<const llvm::Value*, const llvm::Value*>> v_in_first_u_in_second,
								 const llvm::Value* callsite_value);

  /**
   * @brief Returns names of all functions that are represented in this points-to graph.
   *
   * If this is a merged points-to graph, the vector will contain the names of all functions
   * that were merged. Otherwise it will only contain a single function name.
   *
   * @return Vector with function names.
   */
  vector<string> getMergeStack();

  /**
   *
   */
  void printValueVertexMap();

  /**
   * @brief Prints the points-to graph to the command-line.
   */
  void print();

  /**
   * @brief Prints the points-to graph to the command-line.
   */
	void print() const;

  /**
   * @brief Prints the points-to graph as a .dot file.
   * @param filename Filename of the .dot file.
   */
  void printAsDot(const string& filename);

  /**
   *
   * @brief
   */
  void exportPATBCJSON();

  // these are defined in the DBConn class
  friend void operator<<(DBConn& db, const PointsToGraph& PTG);
  friend void operator>>(DBConn& db, PointsToGraph& PTG);
};

#endif /* ANALYSIS_POINTSTOGRAPH_HH_ */
