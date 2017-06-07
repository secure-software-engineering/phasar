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
#include <set>
using namespace std;


class LLVMBasedCFG : public CFG<const llvm::Function*, const llvm::Instruction*> {
public:
	LLVMBasedCFG();
	virtual ~LLVMBasedCFG();

	virtual vector<const llvm::Instruction*> getPredsOf(const llvm::Instruction* u) override;

	virtual vector<const llvm::Instruction*> getSuccsOf(const llvm::Instruction* n) override;

	virtual set<const llvm::Function*> getCalleesOfCallAt(const llvm::Instruction* n) override;

	virtual set<const llvm::Instruction*> getReturnSitesOfCallAt(const llvm::Instruction* n) override;

	virtual bool isCallStmt(const llvm::Instruction* stmt) override;

	virtual bool isExitStmt(const llvm::Instruction* stmt) override;

	virtual bool isStartPoint(const llvm::Instruction* stmt) override;

	virtual set<const llvm::Instruction*> allNonCallStartNodes() override;

	virtual bool isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

	virtual bool isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;
};

#endif /* SRC_ANALYSIS_ICFG_LLVMBASEDCFG_HH_ */
