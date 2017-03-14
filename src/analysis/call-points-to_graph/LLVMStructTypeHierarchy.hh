/*
 * ClassHierarchy.hh
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_
#define ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <tuple>
#include <boost/graph/transitive_closure.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/depth_first_search.hpp>
using namespace std;


class LLVMStructTypeHierarchy {
private:
	typedef boost::property<boost::vertex_name_t, llvm::Type*> Name;
	typedef boost::property<boost::vertex_index_t, std::size_t, Name> Index;
	typedef boost::adjacency_list<boost::setS, boost::vecS, boost::directedS, Index> digraph_t;
	typedef boost::graph_traits<digraph_t>::vertex_descriptor vertex_t;

	set<vertex_t> collected_vertices;
	size_t index = 0;
	digraph_t g;
	map<const llvm::Type*, vertex_t> type_vertex_map;

	struct dfs_tree_edge_visitor : boost::default_dfs_visitor {
		template<class Edge, class Graph>
		void tree_edge(Edge e, const Graph& g) const
		{
			cout << "yeah!" << endl;
			//collected_vertices.insert({boost::target(e, g)});
		}
	};

public:
	LLVMStructTypeHierarchy() = default;
	~LLVMStructTypeHierarchy() = default;
	void analyzeModule(const llvm::Module& M);
	set<const llvm::Type*> getTransitivelyReachableTypes(const llvm::Type* T);
	vector<const llvm::Function*> constructVTable(const llvm::Type* T, const llvm::Module* M);
	const llvm::Function* getFunctionFromVirtualCallSite(llvm::Module* M, llvm::ImmutableCallSite ICS);
	bool containsSubType(const llvm::Type* T, const llvm::Type* ST);
	bool hasSuperType(const llvm::Type* T);
	bool hasSubType(const llvm::Type* T);
	bool containsVTable(const llvm::Type* T);
	void printTransitiveClosure();
	friend ostream& operator<< (ostream& os, const LLVMStructTypeHierarchy& ch);
};

#endif /* ANALYSIS_LLVMSTRUCTTYPEHIERARCHY_HH_ */
