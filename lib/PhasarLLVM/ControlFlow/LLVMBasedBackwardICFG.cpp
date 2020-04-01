/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <memory>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "boost/graph/copy.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/log/sources/record_ostream.hpp"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/CHAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/DTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/OTFResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"

#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMM.h"
#include "phasar/Utils/Utilities.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedBackwardICFG.h"

using namespace psr;
using namespace std;
namespace psr {

LLVMBasedBackwardsICFG::LLVMBasedBackwardsICFG(LLVMBasedICFG &ICFG)
    : ForwardICFG(ICFG) {
  auto cgCopy = ForwardICFG.CallGraph;
  boost::copy_graph(boost::make_reverse_graph(cgCopy), ForwardICFG.CallGraph);
}

LLVMBasedBackwardsICFG::LLVMBasedBackwardsICFG(
    ProjectIRDB &IRDB, CallGraphAnalysisType CGType,
    const std::set<std::string> &EntryPoints, LLVMTypeHierarchy *TH,
    LLVMPointsToInfo *PT, SoundnessFlag SF)
    : ForwardICFG(IRDB, CGType, EntryPoints, TH, PT, SF) {
  auto cgCopy = ForwardICFG.CallGraph;
  boost::copy_graph(boost::make_reverse_graph(cgCopy), ForwardICFG.CallGraph);
}

bool LLVMBasedBackwardsICFG::isCallStmt(const llvm::Instruction *stmt) const {
  return ForwardICFG.isCallStmt(stmt);
}

bool LLVMBasedBackwardsICFG::isIndirectFunctionCall(
    const llvm::Instruction *stmt) const {
  return ForwardICFG.isIndirectFunctionCall(stmt);
}

bool LLVMBasedBackwardsICFG::isVirtualFunctionCall(
    const llvm::Instruction *stmt) const {
  return ForwardICFG.isVirtualFunctionCall(stmt);
}

std::set<const llvm::Function *>
LLVMBasedBackwardsICFG::getAllFunctions() const {
  return ForwardICFG.getAllFunctions();
}

const llvm::Function *
LLVMBasedBackwardsICFG::getFunction(const std::string &fun) const {
  return ForwardICFG.getFunction(fun);
}

std::set<const llvm::Function *>
LLVMBasedBackwardsICFG::getCalleesOfCallAt(const llvm::Instruction *n) const {
  return ForwardICFG.getCalleesOfCallAt(n);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallersOf(const llvm::Function *m) const {
  return ForwardICFG.getCallersOf(m);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getCallsFromWithin(const llvm::Function *m) const {
  return ForwardICFG.getCallsFromWithin(m);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getStartPointsOf(const llvm::Function *m) const {
  return ForwardICFG.getExitPointsOf(m);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getExitPointsOf(const llvm::Function *fun) const {
  return ForwardICFG.getStartPointsOf(fun);
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::getReturnSitesOfCallAt(
    const llvm::Instruction *n) const {
  std::set<const llvm::Instruction *> ReturnSites;
  if (auto Call = llvm::dyn_cast<llvm::CallInst>(n)) {
    if (auto Prev = Call->getPrevNode())
      ReturnSites.insert(Prev);
  }
  if (auto Invoke = llvm::dyn_cast<llvm::InvokeInst>(n)) {
    ReturnSites.insert(&Invoke->getNormalDest()->back());
    ReturnSites.insert(&Invoke->getUnwindDest()->back());
  }
  return ReturnSites;
}

std::set<const llvm::Instruction *>
LLVMBasedBackwardsICFG::allNonCallStartNodes() const {
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

void LLVMBasedBackwardsICFG::print(std::ostream &OS) const {
  ForwardICFG.print(OS);
}

void LLVMBasedBackwardsICFG::printAsDot(std::ostream &OS) const {
  ForwardICFG.printAsDot(OS);
}

void LLVMBasedBackwardsICFG::printInternalPTGAsDot(std::ostream &OS) const {
  ForwardICFG.printInternalPTGAsDot(OS);
}

nlohmann::json LLVMBasedBackwardsICFG::getAsJson() const {
  return ForwardICFG.getAsJson();
}

unsigned LLVMBasedBackwardsICFG::getNumOfVertices() {
  return ForwardICFG.getNumOfVertices();
}

unsigned LLVMBasedBackwardsICFG::getNumOfEdges() {
  return ForwardICFG.getNumOfEdges();
}

const PointsToGraph &LLVMBasedBackwardsICFG::getWholeModulePTG() const {
  return ForwardICFG.getWholeModulePTG();
}

std::vector<const llvm::Function *>
LLVMBasedBackwardsICFG::getDependencyOrderedFunctions() {
  return ForwardICFG.getDependencyOrderedFunctions();
}

} // namespace psr
