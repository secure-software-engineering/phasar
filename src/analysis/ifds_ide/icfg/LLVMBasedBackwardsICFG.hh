/*

 * LLVMBasedBackwardsICFG.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt


#ifndef ANALYSIS_LLVMBASEDBACKWARDSICFG_HH_
#define ANALYSIS_LLVMBASEDBACKWARDSICFG_HH_

#include <vector>
#include <set>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include "ifds_ide/BiDiICFG.hh"

using namespace std;

class LLVMBasedBackwardsICFG : public BiDiICFG<const llvm::Instruction*, const llvm::Function*> {
private:


public:
	LLVMBasedBackwardsICFG();

	virtual ~LLVMBasedBackwardsICFG();

	//swapped
	vector<const llvm::Instruction*> getSuccsOf(const llvm::Instruction* n) override;

	//swapped
	set<const llvm::Instruction*> getStartPointsOf(const llvm::Function* m) override;

	//swapped
	set<const llvm::Instruction*> getReturnSitesOfCallAt(const llvm::Instruction* n) override;

	//swapped
	bool isExitStmt(const llvm::Instruction* stmt) override;

	//swapped
	bool isStartPoint(const llvm::Instruction* stmt) override;

	//swapped
	set<const llvm::Instruction*> allNonCallStartNodes() override;

	//swapped
	vector<const llvm::Instruction*> getPredsOf(const llvm::Instruction* u) override;

	//swapped
	set<const llvm::Instruction*> getEndPointsOf(const llvm::Function* m) override;

	//swapped
	vector<const llvm::Instruction*> getPredsOfCallAt(const llvm::Instruction* u) override;

	//swapped
	set<const llvm::Instruction*> allNonCallEndNodes() override;

	//same
	const llvm::Function* getMethodOf(const llvm::Instruction* n) override;

	//same
	set<const llvm::Function*> getCalleesOfCallAt(const llvm::Instruction* n) override;

	//same
	set<const llvm::Instruction*> getCallersOf(const llvm::Function* m) override;

	//same
	set<const llvm::Instruction*> getCallsFromWithin(const llvm::Function* m) override;

	//same
	bool isCallStmt(const llvm::Instruction* stmt) override;

	//same
	//DirectedGraph<const llvm::Instruction*> getOrCreateUnitGraph(const llvm::Function* m) override;

	//same
	vector<const llvm::Instruction*> getParameterRefs(const llvm::Function* m) override;

	bool isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

	bool isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

	//swapped
	bool isReturnSite(const llvm::Instruction* n) override;

	// same
	bool isReachable(const llvm::Instruction* u) override;

};

#endif  ANALYSIS_LLVMBASEDBACKWARDSICFG_HH_
*/
