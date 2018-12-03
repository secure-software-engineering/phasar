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
namespace psr {

LLVMBasedBackwardsICFG::LLVMBasedBackwardsICFG(LLVMTypeHierarchy &STH,
                                               ProjectIRDB &IRDB)
    : ForwardICFG(STH, IRDB) {
  boost::copy_graph(boost::make_reverse_graph(ForwardICFG.cg), ForwardICFG.cg);
};

LLVMBasedBackwardsICFG::LLVMBasedBackwardsICFG(
    LLVMTypeHierarchy &STH, ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
    const std::vector<std::string> &EntryPoints)
    : ForwardICFG(STH, IRDB, CGType, EntryPoints) {
  boost::copy_graph(boost::make_reverse_graph(ForwardICFG.cg), ForwardICFG.cg);
};

LLVMBasedBackwardsICFG::LLVMBasedBackwardsICFG(
    LLVMTypeHierarchy &STH, ProjectIRDB &IRDB, const llvm::Module &M,
    CallGraphAnalysisType CGType, std::vector<std::string> EntryPoints)
    : ForwardICFG(STH, IRDB, M, CGType, EntryPoints) {
  boost::copy_graph(boost::make_reverse_graph(ForwardICFG.cg), ForwardICFG.cg);
};

bool LLVMBasedBackwardsICFG::isVirtualFunctionCall(llvm::ImmutableCallSite CS) {
  return ForwardICFG.isVirtualFunctionCall(CS);
}

std::set<const llvm::Function *> LLVMBasedBackwardsICFG::getAllMethods() {
  return ForwardICFG.getAllMethods();
}

const llvm::Function *
LLVMBasedBackwardsICFG::getMethod(const std::string &fun) {
  return ForwardICFG.getMethod(fun);
}

std::set<const llvm::Function *>
LLVMBasedBackwardsICFG::getCalleesOfCallAt(const llvm::Instruction *n) {
  return ForwardICFG.getCalleesOfCallAt(n);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallersOf(const llvm::Function *m) {
  return ForwardICFG.getCallersOf(m);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallsFromWithin(const llvm::Function *m) {
  return ForwardICFG.getCallsFromWithin(m);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getStartPointsOf(const llvm::Function *m) {
  return ForwardICFG.getStartPointsOf(m);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getExitPointsOf(const llvm::Function *fun) {
  return ForwardICFG.getExitPointsOf(fun);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getReturnSitesOfCallAt(const llvm::Instruction *n) {
  return ForwardICFG.getReturnSitesOfCallAt(n);
}

bool LLVMBasedBackwardsICFG::isCallStmt(const llvm::Instruction *stmt) {
  return ForwardICFG.isCallStmt(stmt);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::allNonCallStartNodes() {
  return ForwardICFG.allNonCallStartNodes();
}

const llvm::Instruction *
LLVMBasedBackwardsICFG::getLastInstructionOf(const std::string &name) {
  return ForwardICFG.getLastInstructionOf(name);
}

std::vector<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getAllInstructionsOfFunction(const std::string &name) {
  return ForwardICFG.getAllInstructionsOfFunction(name);
}

void LLVMBasedBackwardsICFG::mergeWith(const LLVMBasedBackwardsICFG &other) {
  ForwardICFG.mergeWith(other.ForwardICFG);
}

bool LLVMBasedBackwardsICFG::isPrimitiveFunction(const std::string &name) {
  return ForwardICFG.isPrimitiveFunction(name);
}

void LLVMBasedBackwardsICFG::print() { ForwardICFG.print(); }

void LLVMBasedBackwardsICFG::printAsDot(const std::string &filename) {
  ForwardICFG.printAsDot(filename);
}

void LLVMBasedBackwardsICFG::printInternalPTGAsDot(
    const std::string &filename) {
  ForwardICFG.printInternalPTGAsDot(filename);
}

json LLVMBasedBackwardsICFG::getAsJson() { return ForwardICFG.getAsJson(); }

unsigned LLVMBasedBackwardsICFG::getNumOfVertices() {
  return ForwardICFG.getNumOfVertices();
}

unsigned LLVMBasedBackwardsICFG::getNumOfEdges() {
  return ForwardICFG.getNumOfEdges();
}

void LLVMBasedBackwardsICFG::exportPATBCJSON() {
  return ForwardICFG.exportPATBCJSON();
}

PointsToGraph &LLVMBasedBackwardsICFG::getWholeModulePTG() {
  return ForwardICFG.getWholeModulePTG();
}

std::vector<std::string>
LLVMBasedBackwardsICFG::getDependencyOrderedFunctions() {
  return ForwardICFG.getDependencyOrderedFunctions();
}

} // namespace psr