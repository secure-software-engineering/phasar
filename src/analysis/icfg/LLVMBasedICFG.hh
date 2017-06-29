/*
 * LLVMBasedInterproceduralICFG.hh
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_LLVMBASEDICFG_HH_
#define ANALYSIS_LLVMBASEDICFG_HH_

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <llvm/Support/Casting.h>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
#include <map>
#include <string>
#include "../call-points-to_graph/LLVMStructTypeHierarchy.hh"
#include "../call-points-to_graph/PointsToGraph.hh"
#include "../../lib/GraphExtensions.hh"
#include "../../lib/LLVMShorthands.hh"
#include "../../utils/utils.hh"
#include "ICFG.hh"

using namespace std;

class LLVMBasedICFG : public ICFG<const llvm::Instruction*, const llvm::Function*> {
 private:
  const llvm::Module& M;
  LLVMStructTypeHierarchy& CH;
  ProjectIRCompiledDB& IRDB;
  PointsToGraph WholeModulePTG;
  set<string> VisitedFunctions;
  vector<string> CallStack;
  map<const llvm::Instruction*, const llvm::Function*> DirectCSTargetMethods;
  map<const llvm::Instruction*, set<const llvm::Function*>> IndirectCSTargetMethods;
  /*
   * Additionally to DirectCSTargetMethods and IndirectCSTargetMethods, we store the
   * information as a boost graph to enable the persistent storage via Hexastore more
   * easily. At some point the maps may be replace by this graph.
   */
  struct VertexProperties {
    const llvm::Function* function = nullptr;
    string functionName;
    VertexProperties() = default;
  	VertexProperties(const llvm::Function* f);
  };

  struct EdgeProperties {
    const llvm::Instruction* callsite = nullptr;
    string ir_code;
    size_t id = 0;
    EdgeProperties() = default;
    EdgeProperties(const llvm::Instruction* i);
  };
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                                VertexProperties, EdgeProperties> bidigraph_t;
  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;
  bidigraph_t cg;
  map<string, vertex_t> function_vertex_map;


  set<string> resolveIndirectCall(llvm::ImmutableCallSite CS);
  void printPTGMapping(vector<pair<const llvm::Value*, const llvm::Value*>> mapping);

 public:
  LLVMBasedICFG(llvm::Module& Module,
                LLVMStructTypeHierarchy& STH,
                ProjectIRCompiledDB& IRDB);

  LLVMBasedICFG(llvm::Module& Module,
  							LLVMStructTypeHierarchy& STH,
								ProjectIRCompiledDB& IRDB,
								const vector<string>& EntryPoints);

  virtual ~LLVMBasedICFG() = default;

  void resolveIndirectCallWalker(const llvm::Function* F);

  const llvm::Module& getModule() { return M; }

  virtual const llvm::Function* getMethodOf(const llvm::Instruction* stmt) override;

  vector<const llvm::Instruction*> getPredsOf(const llvm::Instruction* I) override;

  vector<const llvm::Instruction*> getSuccsOf(const llvm::Instruction* I) override;

  vector<pair<const llvm::Instruction*,const llvm::Instruction*>> getAllControlFlowEdges(const llvm::Function* fun) override;

  vector<const llvm::Instruction*> getAllInstructionsOf(const llvm::Function* fun) override;

  bool isExitStmt(const llvm::Instruction* stmt) override;

  bool isStartPoint(const llvm::Instruction* stmt) override;

  bool isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

  bool isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

  string getMethodName(const llvm::Function* fun) override;

  set<const llvm::Function*> getCalleesOfCallAt(
      const llvm::Instruction* n) override;

  set<const llvm::Instruction*> getCallersOf(const llvm::Function* m) override;

  set<const llvm::Instruction*> getCallsFromWithin(
      const llvm::Function* m) override;

  set<const llvm::Instruction*> getStartPointsOf(
      const llvm::Function* m) override;

  set<const llvm::Instruction*> getExitPointsOf(
  		const llvm::Function* fun) override;

  set<const llvm::Instruction*> getReturnSitesOfCallAt(
      const llvm::Instruction* n) override;

  CallType isCallStmt(const llvm::Instruction* stmt) override;

  set<const llvm::Instruction*> allNonCallStartNodes() override;

  const llvm::Instruction* getLastInstructionOf(const string& name);

  vector<const llvm::Instruction*> getAllInstructionsOfFunction(const string& name);

  void print();

  void printAsDot(const string& filename);
};

#endif /* ANALYSIS_LLVMBASEDINTERPROCEDURALCFG_HH_ */
