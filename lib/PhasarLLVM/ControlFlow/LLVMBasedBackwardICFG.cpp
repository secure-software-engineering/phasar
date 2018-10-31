/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/



#include <memory>

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>

#include <boost/graph/copy.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/DTAResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h>
#include <phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h>

#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/Macros.h>
#include <phasar/Utils/PAMM.h>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Pointer/VTable.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h>

using namespace psr;
using namespace std;
namespace psr{


LLVMBasedBackwardsICFG::LLVMBasedBackwardsICFG(LLVMTypeHierarchy &STH, ProjectIRDB &IRDB,CallGraphAnalysisType CGType,const vector<string> &EntryPoints)
 : CGType(CGType), CH(STH), IRDB(IRDB) {

		for (auto &EntryPoint : EntryPoints) {
    llvm::Function *F = IRDB.getFunction(EntryPoint);
    if (F == nullptr) {
      throw ios_base::failure(
          "Could not retrieve llvm::Function for entry point");
    }
    PointsToGraph &ptg = *IRDB.getPointsToGraph(EntryPoint);
    WholeModulePTG.mergeWith(ptg, F);
  }
}

bool LLVMBasedBackwardsICFG::isVirtualFunctionCall(llvm::ImmutableCallSite CS)
{
	return false;
}

const llvm::Function *
LLVMBasedBackwardsICFG::getMethodOf(const llvm::Instruction *stmt) 
{
	return stmt->getFunction();
}

std::vector<const llvm::Instruction *> 
LLVMBasedBackwardsICFG::getPredsOf(const llvm::Instruction *I) 
{
	return std::vector<const llvm::Instruction *>();
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getSuccsOf(const llvm::Instruction *I) 
{
	return std::vector<const llvm::Instruction *>();
}

std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>
LLVMBasedBackwardsICFG::getAllControlFlowEdges(const llvm::Function *fun) 
{
	return std::vector<std::pair<const llvm::Instruction *, const llvm::Instruction *>>();
}

std::set<const llvm::Function *> LLVMBasedBackwardsICFG::getAllMethods()
{
	return std::set<const llvm::Function *>();
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getAllInstructionsOf(const llvm::Function *fun) 
{
	vector<const llvm::Instruction *> Instructions;
    for (auto &BB : *fun) {
      for (auto &I : BB) {
        Instructions.insert(Instructions.begin(),&I);
      }
    }
    return Instructions;
}

bool LLVMBasedBackwardsICFG::isExitStmt(const llvm::Instruction *stmt) 
{
	return (stmt == &stmt->getFunction()->front().front());
}

bool LLVMBasedBackwardsICFG::isStartPoint(const llvm::Instruction *stmt) 
{
	return llvm::isa<llvm::ReturnInst>(stmt);
}

bool LLVMBasedBackwardsICFG::isFallThroughSuccessor(const llvm::Instruction *stmt,
                              const llvm::Instruction *succ) 
{
	return false;
}

bool LLVMBasedBackwardsICFG::isBranchTarget(const llvm::Instruction *stmt,
                      const llvm::Instruction *succ) 
{
	return false;
}

std::string LLVMBasedBackwardsICFG::getMethodName(const llvm::Function *fun) 
{
	return fun->getName().str();
}

std::string LLVMBasedBackwardsICFG::getStatementId(const llvm::Instruction *stmt) 
{
	return llvm::cast<llvm::MDString>(
             stmt->getMetadata(MetaDataKind)->getOperand(0))
      ->getString()
      .str();
}

const llvm::Function *LLVMBasedBackwardsICFG::getMethod(const std::string &fun) 
{
	return nullptr;
}

std::set<const llvm::Function *>
LLVMBasedBackwardsICFG::getCalleesOfCallAt(const llvm::Instruction *n) 
{
	return std::set<const llvm::Function *>();
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallersOf(const llvm::Function *m) 
{
	return std::set<const llvm::Instruction *>();
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallsFromWithin(const llvm::Function *m) 
{
	return std::set<const llvm::Instruction *>();
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getStartPointsOf(const llvm::Function *m) 
{
	return std::set<const llvm::Instruction *>();
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getExitPointsOf(const llvm::Function *fun) 
{
	return std::set<const llvm::Instruction *>();
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getReturnSitesOfCallAt(const llvm::Instruction *n) 
{
	return std::set<const llvm::Instruction *>();
}

bool LLVMBasedBackwardsICFG::isCallStmt(const llvm::Instruction *stmt) 
{
	return false;
}


std::set<const llvm::Instruction *> LLVMBasedBackwardsICFG::allNonCallStartNodes() 
{
	return std::set<const llvm::Instruction *>();
}

const llvm::Instruction *LLVMBasedBackwardsICFG::getLastInstructionOf(const std::string &name)
{
	return nullptr;
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getAllInstructionsOfFunction(const std::string &name)
{
	return std::vector<const llvm::Instruction *>();
}

void LLVMBasedBackwardsICFG::mergeWith(const LLVMBasedBackwardsICFG &other)
{

}

bool LLVMBasedBackwardsICFG::isPrimitiveFunction(const std::string &name)
{
	return false;
}

void LLVMBasedBackwardsICFG::print()
{

}

void LLVMBasedBackwardsICFG::printAsDot(const std::string &filename)
{

}

void LLVMBasedBackwardsICFG::printInternalPTGAsDot(const std::string &filename)
{

}

json LLVMBasedBackwardsICFG::getAsJson() 
{
	return json();
}

unsigned LLVMBasedBackwardsICFG::getNumOfVertices()
{
	return 0;
}

unsigned LLVMBasedBackwardsICFG::getNumOfEdges()
{
	return 0;
}

void LLVMBasedBackwardsICFG::exportPATBCJSON()
{

}

PointsToGraph &LLVMBasedBackwardsICFG::getWholeModulePTG()
{
	return WholeModulePTG;
}

std::vector<std::string> LLVMBasedBackwardsICFG::getDependencyOrderedFunctions()
{
	return std::vector<std::string>();
}

}//namespace psr