/*
 * LLVMBasedInterproceduralICFG.cpp
 *
 *  Created on: 09.09.2016
 *      Author: pdschbrt
 */

#include "LLVMBasedInterproceduralCFG.hh"


const llvm::Function* LLVMBasedInterproceduralICFG::getMethodOf(const llvm::Instruction* n)
{
	return n->getFunction();
}

vector<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getPredsOf(const llvm::Instruction* u)
{
	// FIXME
	cout << "getPredsOf not supported yet" << endl;
	vector<const llvm::Instruction*> IVec;
	const llvm::BasicBlock* BB = u->getParent();
	if (BB->getFirstNonPHIOrDbg() != u) {

	} else {
		IVec.push_back(u->getPrevNode());
	}
	return IVec;
}

vector<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getSuccsOf(const llvm::Instruction* n)
{
	vector<const llvm::Instruction*> IVec;
	// if n is a return instruction, there are no more successors
	if (llvm::isa<llvm::ReturnInst>(n))
			return IVec;
	// get the next instruction
	const llvm::Instruction* I = n->getNextNode();
	// check if the next instruction is not a branch instruction just add it to the successors
	if (!llvm::isa<llvm::BranchInst>(I)) {
		IVec.push_back(I);
	} else {
		// if it is a branch instruction there are multiple successors we have to collect
		const llvm::BranchInst* BI = llvm::dyn_cast<const llvm::BranchInst>(I);
		for (size_t i = 0; i < BI->getNumSuccessors(); ++i)
			IVec.push_back(BI->getSuccessor(i)->getFirstNonPHIOrDbg());
	}
	return IVec;
}

/**
 * Returns all callee methods for a given call.
 */
set<const llvm::Function*> LLVMBasedInterproceduralICFG::getCalleesOfCallAt(const llvm::Instruction* n)
{
	set<const llvm::Function*> CalleesAt;
	if (const llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(n)) {
		CalleesAt.insert(CI->getCalledFunction());
	}
	return CalleesAt;
}

/**
 * Returns all caller statements/nodes of a given method.
 */
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getCallersOf(const llvm::Function* m)
{
	set<const llvm::Instruction*> CallersOf;
	for (llvm::Module::const_iterator MI = M.begin(); MI != M.end(); ++MI) {
		for (llvm::Function::const_iterator FI = MI->begin(); FI != MI->end(); ++FI) {
			llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
			for (llvm::BasicBlock::const_iterator BBI = BB->begin(); BBI != BB->end(); ++BBI) {
				const llvm::Instruction& I = *BBI;
				if (const llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
					if (CI->getCalledFunction() == m) {
						CallersOf.insert(&I);
					}
				}
			}
		}
	}
	return CallersOf;
}

/**
 * Returns all call sites within a given method.
 */
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getCallsFromWithin(const llvm::Function* m)
{
	set<const llvm::Instruction*> ISet;
	for (llvm::Function::const_iterator FI = m->begin(); FI != m->end(); ++FI) {
		llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
		   // iterate over single instructions
		for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
		   	const llvm::Instruction &I = *BI++;
			if (llvm::isa<llvm::CallInst>(I))
				ISet.insert(&I);
		}
	}
	return ISet;
}

/**
 * Returns all start points of a given method. There may be
 * more than one start point in case of a backward analysis.
 */
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getStartPointsOf(const llvm::Function* m)
{
	set<const llvm::Instruction*> StartPoints;
	// this does not handle backwards analysis, where a function may contains more than one start points!
	StartPoints.insert(m->getEntryBlock().getFirstNonPHIOrDbg());
	return StartPoints;
}


/**
 * Returns all statements to which a call could return.
 * In the RHS paper, for every call there is just one return site.
 * We, however, use as return site the successor statements, of which
 * there can be many in case of exceptional flow.
 */
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getReturnSitesOfCallAt(const llvm::Instruction* n)
{
	// at the moment we just ignore exceptional control flow
	set<const llvm::Instruction*> RetSites;
	if (llvm::isa<llvm::CallInst>(n)) {
		RetSites.insert(n->getNextNode());
	}
	return RetSites;
}

bool LLVMBasedInterproceduralICFG::isCallStmt(const llvm::Instruction* stmt)
{
	return llvm::isa<llvm::CallInst>(stmt);
}

bool LLVMBasedInterproceduralICFG::isExitStmt(const llvm::Instruction* stmt)
{
	return llvm::isa<llvm::ReturnInst>(stmt);
}

/**
 * Returns all start points of a given method. There may be
 * more than one start point in case of a backward analysis.
 */
bool LLVMBasedInterproceduralICFG::isStartPoint(const llvm::Instruction* stmt)
{
	const llvm::Function* F = stmt->getFunction();
	return F->getEntryBlock().getFirstNonPHIOrDbg() == stmt;
}

/**
 * Returns the set of all nodes that are neither call nor start nodes.
 */
set<const llvm::Instruction*> LLVMBasedInterproceduralICFG::allNonCallStartNodes()
{
	set<const llvm::Instruction*> NonCallStartNodes;
	for (llvm::Module::const_iterator MI = M.begin(); MI != M.end(); ++MI) {
		for (llvm::Function::const_iterator FI = MI->begin(); FI != MI->end(); ++FI) {
			llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
			for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
		    	const llvm::Instruction &I = *BI++;
		    	if ((!llvm::isa<llvm::CallInst>(I)) && (!isStartPoint(&I)))
		    		NonCallStartNodes.insert(&I);
			}
		}
	}
	return NonCallStartNodes;
}

/**
 * Returns whether succ is the fall-through successor of stmt,
 * i.e., the unique successor that is be reached when stmt
 * does not branch.
 */
bool LLVMBasedInterproceduralICFG::isFallThroughSuccessor(const llvm::Instruction* stmt, const llvm::Instruction* succ)
{
	return (stmt->getParent() == succ->getParent());
}


/**
 * Returns whether succ is a branch target of stmt.
 */
bool LLVMBasedInterproceduralICFG::isBranchTarget(const llvm::Instruction* stmt, const llvm::Instruction* succ)
{
	if (const llvm::BranchInst *BI = llvm::dyn_cast<llvm::BranchInst>(stmt)) {
		for (size_t successor = 0; successor < BI->getNumSuccessors(); ++successor) {
			const llvm::BasicBlock *BB = BI->getSuccessor(successor);
			if (BB == succ->getParent())
				return true;
		}
	}
	return false;
}

vector<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getAllInstructionsOfFunction(const string& name)
{
	vector<const llvm::Instruction*> IVec;
	const llvm::Function* func = M.getFunction(name);
	for (llvm::Function::const_iterator FI = func->begin(); FI != func->end(); ++FI) {
		llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
		for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
	    	const llvm::Instruction &I = *BI++;
	    	IVec.push_back(&I);
		}
	}
	return IVec;
}

const llvm::Instruction* LLVMBasedInterproceduralICFG::getLastInstructionOf(const string& name)
{
	const llvm::Function* func = M.getFunction(name);
	for (llvm::Function::const_iterator FI = func->begin(); FI != func->end(); ++FI) {
		llvm::ilist_iterator<const llvm::BasicBlock> BB = FI;
		for (llvm::BasicBlock::const_iterator BI = BB->begin(), BE = BB->end(); BI != BE;) {
	    	const llvm::Instruction &I = *BI++;
	    	if (llvm::isa<llvm::ReturnInst>(I))
	    		return &I;
		}
	}
	return nullptr;
}

vector<const llvm::Instruction*> LLVMBasedInterproceduralICFG::getAllInstructionsOfFunction(const llvm::Function* func)
{
	return getAllInstructionsOfFunction(func->getName().str());
}

const string LLVMBasedInterproceduralICFG::getNameOfMethod(const llvm::Instruction* stmt)
{
	return stmt->getFunction()->getName().str();
}
