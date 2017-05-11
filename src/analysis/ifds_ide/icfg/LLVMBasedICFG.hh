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
#include <llvm/Support/Casting.h>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include "../../call-points-to_graph/LLVMStructTypeHierarchy.hh"
#include "../../call-points-to_graph/PointsToGraph.hh"
#include "../../../lib/GraphExtensions.hh"
#include "../../../utils/utils.hh"
#include "../SpecialSummaries.hh"
#include "ICFG.hh"

using namespace std;

class LLVMBasedICFG : public ICFG<const llvm::Instruction*, const llvm::Function*> {
 private:
  const llvm::Module& M;
  llvm::CallGraph CG;
  LLVMStructTypeHierarchy& CH;
  ProjectIRCompiledDB& IRDB;
  PointsToGraph WholeModulePTG;
  set<string> VisitedFunctions;
  map<const llvm::Instruction*, const llvm::Function*> DirectCSTargetMethods;
  map<const llvm::Instruction*, set<const llvm::Function*>> IndirectCSTargetMethods;

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

  const llvm::Function* getMethodOf(const llvm::Instruction* n) override;

  vector<const llvm::Instruction*> getPredsOf(
      const llvm::Instruction* u) override;

  vector<const llvm::Instruction*> getSuccsOf(
      const llvm::Instruction* n) override;

  set<const llvm::Function*> getCalleesOfCallAt(
      const llvm::Instruction* n) override;

  set<const llvm::Instruction*> getCallersOf(const llvm::Function* m) override;

  set<const llvm::Instruction*> getCallsFromWithin(
      const llvm::Function* m) override;

  set<const llvm::Instruction*> getStartPointsOf(
      const llvm::Function* m) override;

  set<const llvm::Instruction*> getReturnSitesOfCallAt(
      const llvm::Instruction* n) override;

  CallType isCallStmt(const llvm::Instruction* stmt) override;

  bool isExitStmt(const llvm::Instruction* stmt) override;

  bool isStartPoint(const llvm::Instruction* stmt) override;

  set<const llvm::Instruction*> allNonCallStartNodes() override;

  bool isFallThroughSuccessor(const llvm::Instruction* stmt,
                              const llvm::Instruction* succ) override;

  bool isBranchTarget(const llvm::Instruction* stmt,
                      const llvm::Instruction* succ) override;

  vector<const llvm::Instruction*> getAllInstructionsOfFunction(
      const string& name);

  vector<const llvm::Instruction*> getAllInstructionsOfFunction(
      const llvm::Function* func);

  const llvm::Instruction* getLastInstructionOf(const string& name);

  const string getNameOfMethod(const llvm::Instruction* stmt);

  void print();
};

#endif /* ANALYSIS_LLVMBASEDINTERPROCEDURALCFG_HH_ */
