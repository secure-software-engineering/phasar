/*
 * LLVMBasedCFG.hh
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_ICFG_LLVMBASEDCFG_HH_
#define SRC_ANALYSIS_ICFG_LLVMBASEDCFG_HH_

#include "CFG.hh"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <iostream>
#include <vector>
#include <string>
#include <set>
using namespace std;


class LLVMBasedCFG : public CFG<const llvm::Function*, const llvm::Instruction*> {
private:
	const llvm::Function* F;

public:
	LLVMBasedCFG(const llvm::Function* f);

	virtual ~LLVMBasedCFG() = default;

	virtual vector<const llvm::Instruction*> getPredsOf(const llvm::Instruction* n) override;

	virtual vector<const llvm::Instruction*> getSuccsOf(const llvm::Instruction* n) override;

	virtual vector<pair<const llvm::Instruction*,const llvm::Instruction*>> getAllControlFlowEdges() override;

	virtual vector<const llvm::Instruction*> getAllInstructions() override;

	virtual bool isCallStmt(const llvm::Instruction* stmt) override;

	virtual bool isExitStmt(const llvm::Instruction* stmt) override;

	virtual bool isStartPoint(const llvm::Instruction* stmt) override;

	virtual vector<const llvm::Instruction*> allNonCallStartNodes() override;

	virtual bool isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

	virtual bool isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

	void print();
};

#endif /* SRC_ANALYSIS_ICFG_LLVMBASEDCFG_HH_ */
