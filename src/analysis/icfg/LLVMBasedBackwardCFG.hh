/*
 * LLVMBasedBackwardCFG.hh
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_ICFG_LLVMBASEDBACKWARDCFG_HH_
#define SRC_ANALYSIS_ICFG_LLVMBASEDBACKWARDCFG_HH_

#include "CFG.hh"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <iostream>
#include <vector>
#include <set>
using namespace std;


class LLVMBasedBackwardCFG : public CFG<const llvm::Instruction*, const llvm::Function*> {
public:
	LLVMBasedBackwardCFG();

	virtual ~LLVMBasedBackwardCFG();

	virtual const llvm::Function* getMethodOf(const llvm::Instruction* stmt) override;

	virtual vector<const llvm::Instruction*> getPredsOf(const llvm::Instruction* stmt) override;

	virtual vector<const llvm::Instruction*> getSuccsOf(const llvm::Instruction* stmt) override;

	virtual vector<pair<const llvm::Instruction*,const llvm::Instruction*>> getAllControlFlowEdges(const llvm::Function* fun) override;

	virtual vector<const llvm::Instruction*> getAllInstructionsOf(const llvm::Function* fun) override;

	virtual bool isExitStmt(const llvm::Instruction* stmt) override;

	virtual bool isStartPoint(const llvm::Instruction* stmt) override;

	virtual bool isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

	virtual bool isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ) override;

	virtual string getMethodName(const llvm::Function* fun) override;
};

#endif /* SRC_ANALYSIS_ICFG_LLVMBASEDBACKWARDCFG_HH_ */
